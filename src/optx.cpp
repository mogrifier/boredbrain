#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>

#include "plugin.hpp"
#include <audio.hpp>
#include <context.hpp>
#include <fstream>

constexpr int NUM_AUDIO_INPUTS = 8;
constexpr int NUM_AUDIO_OUTPUTS = 8;

struct Optx : Module {
float calibration[8] = {0}; // example: 8 calibration values
	//dsp::DoubleRingBuffer<dsp::Frame<NUM_AUDIO_INPUTS>, 32768> engineInputBuffer;

//I am removing all of the old led code and vu meter code since I want to use my own light code instead


    enum InputIds {
        ENUMS(AUDIO_INPUTS, 8),
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(AUDIO_OUTPUTS, 8),
        NUM_OUTPUTS
    };


	enum LightId {
		LED1A_LIGHT,
		LED1B_LIGHT,
		LED2A_LIGHT,
		LED2B_LIGHT,
		LED3A_LIGHT,
		LED3B_LIGHT,
		LED4A_LIGHT,
		LED4B_LIGHT,
		LED5A_LIGHT,
		LED5B_LIGHT,
		LED6A_LIGHT,
		LED6B_LIGHT,
		LED7A_LIGHT,
		LED7B_LIGHT,
		LED8A_LIGHT,
		LED8B_LIGHT,
		LIGHTS_LEN
	};


	dsp::ClockDivider lightDivider;

	Optx() {
		//this command configures the number of inputs, outputs and lights for the module. These are arrays.
		config(0, NUM_INPUTS, NUM_OUTPUTS, LIGHTS_LEN);
	    
		for (int i = 0; i < NUM_AUDIO_INPUTS; i++)
			configInput(AUDIO_INPUTS + i, string::f("Audio/CV %d", i + 1));
		for (int i = 0; i < NUM_AUDIO_OUTPUTS; i++)
			configOutput(AUDIO_OUTPUTS + i, string::f("Audio/CV %d", i + 1));

		lightDivider.setDivision(512);

	}



	void process(const ProcessArgs& args) override {
//DEBUG("%p: in process. fill them logs", this);
		// Audio I/O

		/*
		Check if input is connected. readits input and write to corresponding output.
		For now just pass audio straight through from input to output.
		
		*/
		for (int i = 0; i < NUM_AUDIO_INPUTS; i++) {
			if (inputs[AUDIO_INPUTS + i].isConnected() && outputs[AUDIO_OUTPUTS + i].isConnected()) {
				//calibrate the input
				float out = inputs[AUDIO_INPUTS + i].getVoltage() + calibration[i];
				outputs[AUDIO_OUTPUTS + i].setVoltage(out);
				outputs[AUDIO_OUTPUTS + i].setChannels(1);

				//turn on lights- only process every 512 calls
				if (lightDivider.process()) {
					lights[LED1A_LIGHT + i * 2].setBrightness(1.0f);
					lights[LED1B_LIGHT + i * 2].setBrightness(1.0f);
				}
			}
			else {
				outputs[AUDIO_OUTPUTS + i].setVoltage(0.0f);
				outputs[AUDIO_OUTPUTS + i].setChannels(1);

				//turn off lights- only process every 512 calls
				if (lightDivider.process()) {
					lights[LED1A_LIGHT + i * 2].setBrightness(0.0f);
					lights[LED1B_LIGHT + i * 2].setBrightness(0.0f);
				}
			}
		}


		// Lights

		/*Optx also provides visual signal indication of the 8 decoded ADAT channels feeding the OUT 1-8 jacks. Each pair of WHITE LEDs 
		indicates the polarity (-Ve left, +Ve right) and relative strength of the signal (± 10 V).

		Audio-rate signals will generally light both LEDs continuously, whereas LFOs will alternate more apparently between the two. 
		Gates, triggers and positive-going envelopes will generally light only the right LED (+Ve).
		When no signal is present, both LEDs will be off. 
		*/


		/* periodically process light data. */
		if (lightDivider.process()) {
			float lightTime = args.sampleTime * lightDivider.getDivision();
			/* use this code to figure out roughly how to do the lights then refine it.
			// Audio-2: VU meter
			if (NUM_AUDIO_INPUTS == 2) {
				for (int i = 0; i < NUM_AUDIO_INPUTS; i++) {
					lights[VU_LIGHTS + i * 6 + 0].setBrightness(vuMeter[i].getBrightness(0, 0));
				}
			}
			// Audio-8 and Audio-16: pair state lights
			else {
				// Turn on light if at least one port is enabled in the nearby pair.
				for (int i = 0; i < NUM_AUDIO_INPUTS / 2; i++) {
					bool active = port.deviceNumOutputs >= 2 * i + 1;
					bool clip = inputClipTimers[i] > 0.f;
					if (clip)
						inputClipTimers[i] -= lightTime;
					lights[INPUT_LIGHTS + i * 2 + 0].setBrightness(active && !clip);
					lights[INPUT_LIGHTS + i * 2 + 1].setBrightness(active && clip);
				}
				for (int i = 0; i < NUM_AUDIO_OUTPUTS / 2; i++) {
					bool active = port.deviceNumInputs >= 2 * i + 1;
					bool clip = outputClipTimers[i] > 0.f;
					if (clip)
						outputClipTimers[i] -= lightTime;
					lights[OUTPUT_LIGHTS + i * 2 + 0].setBrightness(active & !clip);
					lights[OUTPUT_LIGHTS + i * 2 + 1].setBrightness(active & clip);
				}
			}
			*/

			//FIXME : implement my own light code here

		}
	}

	//need to use these methods to write the calibration tables for each channel (or is it a preset??)
	
	json_t* dataToJson() override {
		DEBUG("%p: in dataToJson", this);
		// Call base class implementation first
		json_t* rootJ = Module::dataToJson();

		// Add a string
		json_object_set_new(rootJ, "myText", json_string("Hello Rack"));

		// Add an integer
		json_object_set_new(rootJ, "myValue", json_integer(123));

		// Add an array
		json_t* arrJ = json_array();
		for (int i = 0; i < 4; i++) {
			json_array_append_new(arrJ, json_integer(i * 10));
		}
		json_object_set_new(rootJ, "myArray", arrJ);

		return rootJ;
	}

	/*
	void dataFromJson(json_t* rootJ) override {
		json_t* audioJ = json_object_get(rootJ, "optxaudio");
		if (audioJ)
			port.fromJson(audioJ);

		json_t* dcFilterJ = json_object_get(rootJ, "optxdcFilter");
		if (dcFilterJ)
			dcFilterEnabled = json_boolean_value(dcFilterJ);
	}
	*/



	    // Save calibration to flat file
    void saveCalibration(const std::string& filename) {
        std::ofstream out(filename);
        if (!out.is_open()) return;
        for (int i = 0; i < 8; i++) {
            out << calibration[i] << "\n";
        }
        out.close();
    }

    // Load calibration from flat file
    void loadCalibration(const std::string& filename) {
        std::ifstream in(filename);
        if (!in.is_open()) return;
        for (int i = 0; i < 8; i++) {
            float val;
            if (in >> val) {
                calibration[i] = val;
            }
        }
        in.close();
    }
};


//this creates my custom boredbrain port and also eliminates the shadow surrounding it
struct MyPort : PortWidget {
    MyPort() {
        // Load your custom SVG
        auto svg = APP->window->loadSvg(asset::plugin(pluginInstance, "res/BrainPort.svg"));
        // Create an SvgWidget child to render it
        auto sw = new SvgWidget();
        sw->setSvg(svg);
        sw->box.pos = Vec(0, 0);
        box.size = sw->box.size;
        addChild(sw);
    }

    void draw(const DrawArgs& args) override {
        // Do NOT call PortWidget::draw(args) → avoids the circular shadow
        Widget::draw(args); // draws children (your SvgWidget)
        // If you want cable indicators, you can draw them manually here
    }
};


struct OptxWidget : ModuleWidget {
	OptxWidget(Optx* module) {

		setModule(module);

		setPanel(createPanel(asset::plugin(pluginInstance, "res/optx.svg")));

		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		//input ports
		addInput(createInputCentered<MyPort>(mm2px(Vec(33.039, 14.421)), module, Optx::AUDIO_INPUTS + 0));
		addInput(createInputCentered<MyPort>(mm2px(Vec(33.039, 26.855)), module, Optx::AUDIO_INPUTS + 1));
		addInput(createInputCentered<MyPort>(mm2px(Vec(33.039, 39.288)), module, Optx::AUDIO_INPUTS + 2));
		addInput(createInputCentered<MyPort>(mm2px(Vec(33.039, 51.722)), module, Optx::AUDIO_INPUTS + 3));
		addInput(createInputCentered<MyPort>(mm2px(Vec(33.039, 64.156)), module, Optx::AUDIO_INPUTS + 4));
		addInput(createInputCentered<MyPort>(mm2px(Vec(33.039, 76.589)), module, Optx::AUDIO_INPUTS + 5));
		addInput(createInputCentered<MyPort>(mm2px(Vec(33.039, 89.023)), module, Optx::AUDIO_INPUTS + 6));
		addInput(createInputCentered<MyPort>(mm2px(Vec(33.039, 101.457)), module, Optx::AUDIO_INPUTS + 7));

		//output ports
		addOutput(createOutputCentered<MyPort>(mm2px(Vec(7.634, 14.221)), module, Optx::AUDIO_OUTPUTS + 0));
		addOutput(createOutputCentered<MyPort>(mm2px(Vec(7.634, 26.855)), module, Optx::AUDIO_OUTPUTS + 1));
		addOutput(createOutputCentered<MyPort>(mm2px(Vec(7.634, 39.288)), module, Optx::AUDIO_OUTPUTS + 2));
		addOutput(createOutputCentered<MyPort>(mm2px(Vec(7.634, 51.722)), module, Optx::AUDIO_OUTPUTS + 3));
		addOutput(createOutputCentered<MyPort>(mm2px(Vec(7.634, 64.156)), module, Optx::AUDIO_OUTPUTS + 4));
		addOutput(createOutputCentered<MyPort>(mm2px(Vec(7.634, 76.589)), module, Optx::AUDIO_OUTPUTS + 5));
		addOutput(createOutputCentered<MyPort>(mm2px(Vec(7.634, 89.023)), module, Optx::AUDIO_OUTPUTS + 6));
		addOutput(createOutputCentered<MyPort>(mm2px(Vec(7.634, 101.257)), module, Optx::AUDIO_OUTPUTS + 7));

		//status lights for each of the 8 channels, 2 lights per channel (just for the output)
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.78, 14.45)), module, Optx::LED1A_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(22.903, 14.45)), module, Optx::LED1B_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.78, 26.899)), module, Optx::LED2A_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(22.903, 26.899)), module, Optx::LED2B_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.78, 39.348)), module, Optx::LED3A_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(22.903, 39.348)), module, Optx::LED3B_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.78, 51.797)), module, Optx::LED4A_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(22.903, 51.797)), module, Optx::LED4B_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.78, 64.246)), module, Optx::LED5A_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(22.903, 64.246)), module, Optx::LED5B_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.78, 76.695)), module, Optx::LED6A_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(22.903, 76.695)), module, Optx::LED6B_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.78, 89.145)), module, Optx::LED7A_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(22.903, 89.145)), module, Optx::LED7B_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.78, 101.594)), module, Optx::LED8A_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(22.903, 101.594)), module, Optx::LED8B_LIGHT));

	}

	//need to 
	
	void appendContextMenu(Menu* menu) override {
		Optx* module = dynamic_cast<Optx*>(this->module);
		assert(module);
		// Calibration submenu
		menu->addChild(new MenuSeparator);
		 // Rack 2: MenuLabel must be constructed then text set
        MenuLabel* label = new MenuLabel();
        label->text = "Calibration";
        menu->addChild(label);

        // Save item
        struct SaveItem : MenuItem {
            Optx* module;
            void onAction(const event::Action& e) override {
                std::string path = asset::user("calibration.txt");
                module->saveCalibration(path);
                DEBUG("Calibration saved to %s", path.c_str());
            }
        };
        SaveItem* saveItem = new SaveItem;
        saveItem->text = "Save Calibration";
        saveItem->module = module;
        menu->addChild(saveItem);

        // Load item
        struct LoadItem : MenuItem {
            Optx* module;
            void onAction(const event::Action& e) override {
                std::string path = asset::user("calibration.txt");
                module->loadCalibration(path);
                DEBUG("Calibration loaded from %s", path.c_str());
            }
        };
        LoadItem* loadItem = new LoadItem;
        loadItem->text = "Load Calibration";
        loadItem->module = module;
        menu->addChild(loadItem);

	}
		
	
};


Model* modelOptx = createModel<Optx, OptxWidget>("OptxCalibrator");


