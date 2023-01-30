/*

Project: OscPocketD/VASynth - Daisy Pod Version
Description: polyphonic MIDI Virtual Analog synthsizer for OscPcoketD/Base2 (Daisy Seed)
Author: Staffan Melin, staffan.melin@oscillator.se
License: GNU General Public License v3.0
Version: 202109
Project site: http://www.oscillator.se/opensource

EDIT: This code was modified by Steven @moonfriendsynth, who doesn't know shit about shit when it 
comes to C++, but I tried my best. So please be cautious as you read/use this 
code. If you find yourself wondering, "why did he do x, y, or z?" just assume that Steve is saying
"Man, I dunno, good question." I did my best to copy code from either Daisy examples or other open
source projects. Please feel free to take this code and make it better or use it for whatever.

EDIT: 1/22/22 v3.2.3 This code was modified again by Christopher @Nettech.
It runs on the Daisy Seed and no longer requires a Daisy Pod board to operate.
Synth parameters are now controlled by a Miditech i2-61 midi keyboard.
Multiple Daisy Seeds will appear as USB-MIDI devices with the name "Daisy Seed Built in" and the device number.
Audio output/input is thru the built-in audio codec.
- Added Audio Input PassThru and MIDI indicator. Upgraded DaisySP to Infrasonic MoogLadder code.
- Added synthesized PW using the VASynth::RAMP wave.
- Added Param Switch and Data Entry Slider. Added common parameters. 
- Added QSPI storage for eight user patches. Made eight selectable presets.
- Added USB-MIDI input. Added Stereo Simulator. Added Moogladder filter. 
- Added Dynamic Voice Assignment. Added velocity control to VCA and VCF.

*/

#include "daisy_seed.h"
#include "daisysp.h"

#include "main.h"
#include "vasynth.h"

using namespace daisy;
using namespace daisysp;

// globals
DaisySeed hardware;
MidiUsbHandler midi;
float sysSampleRate;
float sysCallbackRate;
extern uint8_t preset_max;
extern VASynthSetting preset_setting[PRESET_MAX];

uint8_t param_;

// sound
VASynth vasynth;
uint8_t gPlay = PLAY_ON;

// audio callback

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{	
	float voice_left, voice_right;
	
	for (size_t n = 0; n < size; n += 2)
	{	
		if (gPlay == PLAY_ON)
		{			
			vasynth.Process(&voice_left, &voice_right);
			
			out[n] = voice_left + in[n];;
			out[n + 1] = voice_right + in[n + 1];;	
		} 
		else 
		{
			out[n] = 0;
			out[n + 1] = 0;		
		}		
	}
}

// midi handler
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            if ((vasynth.midi_channel_ == MIDI_CHANNEL_ALL) || (p.channel == vasynth.midi_channel_))
			{
				vasynth.NoteOn(p.note, p.velocity);
				hardware.SetLed(true);
			}
	        break;
        }
        case NoteOff:
        {
            NoteOnEvent p = m.AsNoteOn();
            if ((vasynth.midi_channel_ == MIDI_CHANNEL_ALL) || (p.channel == vasynth.midi_channel_))
			{
				vasynth.NoteOff(p.note);
				hardware.SetLed(false);
			}			
	        break;
        } 
        case ControlChange:
        {
            ControlChangeEvent p = m.AsControlChange();
            switch(p.control_number)
            {
				case 1: // Modulation Wheel
					vasynth.lfo_amp_ = ((float)p.value / 127.0f);
                    vasynth.SetLFO();
                    break;
				case 7: // Data Slider Default (Volume)
					switch(param_)
					{
						case 2:
						{
							vasynth.waveform_ = p.value >> 4;
							vasynth.SetWaveform();
							break;
						}
						case 3:
						{
							vasynth.osc2_waveform_ = p.value >> 4;
							vasynth.SetWaveform();
							break;
						}
						case 4:
						{
							vasynth.osc_mix_ = ((float)p.value / 127.0f);
							break;
						}
						case 5:
						{
							vasynth.osc2_detune_ = ((float)p.value / 127.0f);
							break;
						}
						case 6:
						{
							vasynth.osc2_transpose_ = (1.0f + ((float)p.value / 127.0f));
							break;
						}
						case 7:
						{
							vasynth.filter_res_ = ((float)p.value / 127.0f);
                    		vasynth.SetFilter();
							break;
						}
						case 8:
						{
							vasynth.osc_pw_ = ((float)p.value / 127.0f);
							vasynth.osc2_pw_ = ((float)p.value / 127.0f);
							break;
						}
						case 9:
						{
							vasynth.eg_f_attack_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
						case 10:
						{
							vasynth.eg_f_decay_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
						case 11:
						{
							vasynth.eg_f_sustain_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
						case 12:
						{
							vasynth.eg_f_release_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
						case 13:
						{
							vasynth.eg_a_attack_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
						case 14:
						{
							vasynth.eg_a_decay_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
						case 15:
						{
							vasynth.eg_a_sustain_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
						case 16:
						{
							vasynth.eg_a_release_ = ((float)p.value / 127.0f);
							vasynth.SetEG();
							break;
						}
						case 19:
						{
							vasynth.vel_select_ = p.value >> 5;
							break;
						}
						case 20:
						{
							vasynth.eg_f_amount_ = ((float)p.value / 127.0f);
							break;
						}
						case 21:
						{
							vasynth.lfo_freq_ = ((float)p.value / 127.0f);
							vasynth.SetLFO();
							break;
						}
					}
					break;
                default: break;
            }
            break;
        }
		case ProgramChange:
        {
            ProgramChangeEvent p = m.AsProgramChange();
            if ((vasynth.midi_channel_ == MIDI_CHANNEL_ALL) || (p.channel == vasynth.midi_channel_))
			{
				if(p.program >= 29)
				{
					switch (p.program)
					{
						case 29:
						{
							vasynth.SaveToLive(&preset_setting[0]);
							break;
						}
						case 30:
						{
							vasynth.SaveToLive(&preset_setting[1]);
							break;
						}
						case 31:
						{
							vasynth.SaveToLive(&preset_setting[2]);
							break;
						}
						case 32:
						{
							vasynth.SaveToLive(&preset_setting[3]);
							break;
						}
						case 33:
						{
							vasynth.SaveToLive(&preset_setting[4]);
							break;
						}
						case 34:
						{
							vasynth.SaveToLive(&preset_setting[5]);
							break;
						}
						case 35:
						{
							vasynth.SaveToLive(&preset_setting[6]);
							break;
						}
						case 36:
						{
							vasynth.SaveToLive(&preset_setting[7]);
							break;
						}
						case 39:
						{
							vasynth.FlashLoad(0);
							break;
						}
						case 40:
						{
							vasynth.FlashLoad(1);
							break;
						}
						case 41:
						{
							vasynth.FlashLoad(2);
							break;
						}
						case 42:
						{
							vasynth.FlashLoad(3);
							break;
						}
						case 43:
						{
							vasynth.FlashLoad(4);
							break;
						}
						case 44:
						{
							vasynth.FlashLoad(5);
							break;
						}
						case 45:
						{
							vasynth.FlashLoad(6);
							break;
						}
						case 46:
						{
							vasynth.FlashLoad(7);
							break;
						}
						case 49:
						{
							vasynth.FlashSave(0);
							break;
						}
						case 50:
						{
							vasynth.FlashSave(1);
							break;
						}
						case 51:
						{
							vasynth.FlashSave(2);
							break;
						}
						case 52:
						{
							vasynth.FlashSave(3);
							break;
						}
						case 53:
						{
							vasynth.FlashSave(4);
							break;
						}
						case 54:
						{
							vasynth.FlashSave(5);
							break;
						}
						case 55:
						{
							vasynth.FlashSave(6);
							break;
						}
						case 56:
						{
							vasynth.FlashSave(7);
							break;
						}
					}
				}	
				else
				{
					vasynth.ProgramChange(p.program);
					break;				
				}
			}    
        }
        default: break;
    }
}

int main(void)
{
	// init hardware
	hardware.Init(true); // true = boost to 480MHz

	sysSampleRate = hardware.AudioSampleRate();
	sysCallbackRate = hardware.AudioCallbackRate();

	// init qspi flash for saving and loading patches
	QSPIHandle::Config qspi_config;
	qspi_config.device = QSPIHandle::Config::Device::IS25LP064A;
	qspi_config.mode   = QSPIHandle::Config::Mode::MEMORY_MAPPED;
	qspi_config.pin_config.io0 = {DSY_GPIOF, 8};
	qspi_config.pin_config.io1 = {DSY_GPIOF, 9};
	qspi_config.pin_config.io2 = {DSY_GPIOF, 7};
	qspi_config.pin_config.io3 = {DSY_GPIOF, 6};
	qspi_config.pin_config.clk = {DSY_GPIOF, 10};
	qspi_config.pin_config.ncs = {DSY_GPIOG, 6};
	hardware.qspi.Init(qspi_config);

	// setup incl default values
	vasynth.First();

	// Initialize USB Midi 
    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
	midi.Init(midi_cfg);

	// let everything settle
	System::Delay(100);
	
	// Start calling the audio callback
	hardware.StartAudio(AudioCallback);

	// Loop forever
	for(;;)
	{
        // handle MIDI Events
        midi.Listen();
        while(midi.HasEvents())
        {
            HandleMidiMessage(midi.PopEvent());
        }	
	}
}