#include "ML_modules.hpp"

#include <sstream>
#include <iomanip>

struct Counter : Module {
	enum ParamIds {
		MAX_PARAM,
		START_PARAM,
		STOP_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		LENGTH_INPUT,
		GATE_INPUT,
		START_INPUT,
		STOP_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		START_OUTPUT,
		STOP_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Counter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS); 
        configParam(Counter::MAX_PARAM, 0.0, 128.0, 8.0);
        configParam(Counter::START_PARAM, 0.0, 10.0, 0.0);
        configParam(Counter::STOP_PARAM, 0.0, 10.0, 0.0);

		onReset(); };

	void process(const ProcessArgs &args) override;

	void onReset() override {counter=0; state=false; state2=false; };

	int counter = 0;
	bool state = false;
	bool state2 = false;
	float stateLight;

	float gSampleRate;

	// void onSampleRateChange() override {gSampleRate = args.sampleRate;}

    int max;
  
	dsp::SchmittTrigger startTrigger, gateTrigger, stopTrigger;
	dsp::PulseGenerator startPulse, stopPulse;
};



void Counter::process(const ProcessArgs &args) {


	max = params[MAX_PARAM].getValue();


	if( inputs[LENGTH_INPUT].isConnected() ) max = max * clamp(inputs[LENGTH_INPUT].getVoltage()/10.0f,0.0f,1.0f);

	if( startTrigger.process(inputs[START_INPUT].getNormalVoltage(0.0) + params[START_PARAM].getValue() )) {
		state=true; 
		counter=gateTrigger.isHigh()?1:0;
		startPulse.trigger(0.001);
	};

	if( stopTrigger.process(inputs[STOP_INPUT].getNormalVoltage(0.0) + params[STOP_PARAM].getValue() ))   {
		state=false; 
		counter=0;
		stopPulse.trigger(0.001);
	};

	if( gateTrigger.process(inputs[GATE_INPUT].getNormalVoltage(0.0) ) ) {

		if(state) counter++; 

		if(counter > max) {
			
			counter = 0;
			state   = false;
			stopPulse.trigger(0.001);
		};
		
	};


	float out = (gateTrigger.isHigh()&&state) ? 10.0 : 0.0;

	float start = (startPulse.process(args.sampleTime)) ? 10.0 : 0.0;
	float stop  = (stopPulse.process(args.sampleTime)) ? 10.0 : 0.0;

	outputs[GATE_OUTPUT].value  = out;
	outputs[START_OUTPUT].setVoltage(start);
	outputs[STOP_OUTPUT].value  = stop;

};



struct CounterWidget : ModuleWidget {
	CounterWidget(Counter *module);
};

CounterWidget::CounterWidget(Counter *module) {
		setModule(module);
	box.size = Vec(15*6, 380);

	{
		SvgPanel *panel = new SvgPanel();
		panel->box.size = box.size;
		panel->setBackground(APP->window->loadSvg(asset::plugin(pluginInstance,"res/Counter.svg")));
		addChild(panel);
	}


	

	addChild(createWidget<MLScrew>(Vec(15, 0)));
	addChild(createWidget<MLScrew>(Vec(15, 365)));

	addParam(createParam<SmallBlueMLKnob>(Vec(12,  85), module, Counter::MAX_PARAM));
	addInput(createInput<MLPort>( Vec(53, 87), module, Counter::LENGTH_INPUT));

	addInput(createInput<MLPort>(  Vec(13, 168), module, Counter::GATE_INPUT));
	addOutput(createOutput<MLPort>(Vec(53, 168), module, Counter::GATE_OUTPUT));


	addInput(createInput<MLPort>(  Vec(13, 241), module, Counter::START_INPUT));
	addOutput(createOutput<MLPort>(Vec(53, 241), module, Counter::START_OUTPUT));
	addParam(createParam<MLSmallButton>(   Vec(58, 222), module, Counter::START_PARAM));

	addInput(createInput<MLPort>(  Vec(13, 312), module, Counter::STOP_INPUT));
	addOutput(createOutput<MLPort>(Vec(53, 312), module, Counter::STOP_OUTPUT));
	addParam(createParam<MLSmallButton>(   Vec(58, 293), module, Counter::STOP_PARAM));

	NumberDisplayWidget *display = new NumberDisplayWidget();
	display->box.pos = Vec(20,56);
	display->box.size = Vec(50, 20);
	if(module) display->value = &module->max;
	addChild(display);

	
	NumberDisplayWidget *display2 = new NumberDisplayWidget();
	display2->box.pos = Vec(20,145);
	display2->box.size = Vec(50, 20);
	if(module) display2->value = &module->counter;
	addChild(display2);

}

Model *modelCounter = createModel<Counter, CounterWidget>("Counter");
