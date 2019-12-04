#ifndef WIDGETS_H
#define WIDGETS_H
#include "../ZZC/src/widgets.hpp"
#endif

#include <ctime>

#include "ZZC.hpp"
#include "Phaseque.hpp"
#include "PhasequeWidget.hpp"
#include "helpers.hpp"

void Phaseque::setPolyMode(PolyphonyModes polyMode) {
  if (polyMode == this->polyphonyMode) { return; }
  this->polyphonyMode = polyMode;
}

void Phaseque::goToPattern(unsigned int targetIdx) {
  unsigned int targetIdxSafe = eucMod(targetIdx, NUM_PATTERNS);
  this->storeCurrentPattern();
  this->patternIdx = targetIdxSafe;
  this->takeOutCurrentPattern();
}

void Phaseque::goToFirstNonEmpty() {
  for (int i = 0; i < NUM_PATTERNS; i++) {
    if (this->patterns[i].hasCustomSteps()) {
      this->goToPattern(i);
      return;
    }
  }
}

void Phaseque::jumpToStep(Step step) {
  phase = eucMod((direction == 1 ? step.in() : step.out()) - phaseParam , 1.0f);
  jump = true;
}

void Phaseque::processGlobalParams() {
  // Gates
  if (inputs[GLOBAL_GATE_INPUT].isConnected()) {
    globalGate = globalGateInternal ^ (inputs[GLOBAL_GATE_INPUT].getVoltage() > 1.0f);
  } else {
    globalGate = globalGateInternal;
  }

  // Shift
  if (inputs[GLOBAL_SHIFT_INPUT].isConnected()) {
    globalShift = params[GLOBAL_SHIFT_PARAM].getValue() + clamp(inputs[GLOBAL_SHIFT_INPUT].getVoltage() * 0.2f * baseStepLen, -baseStepLen, baseStepLen);
  } else {
    globalShift = params[GLOBAL_SHIFT_PARAM].getValue();
  }

  // Length
  if (inputs[GLOBAL_LEN_INPUT].isConnected()) {
    globalLen = params[GLOBAL_LEN_PARAM].getValue() * (clamp(inputs[GLOBAL_LEN_INPUT].getVoltage(), -5.0f, 4.999f) * 0.2f + 1.0f);
  } else {
    globalLen = params[GLOBAL_LEN_PARAM].getValue();
  }
}

void Phaseque::processPatternNav() {
  if (waitButtonTrigger.process(params[WAIT_SWITCH_PARAM].getValue())) {
    wait ^= true;
  }
  if (this->wait) {
    lights[WAIT_LED].value = 1.1f;
  }
  if (this->gridDisplayProducer->hasNextPatternRequest) {
    this->pattern.goTo = this->gridDisplayProducer->nextPatternRequest;
    this->gridDisplayProducer->hasNextPatternRequest = false;
  }
  if (this->gridDisplayProducer->hasGoToRequest) {
    unsigned int goToRequest = this->gridDisplayProducer->goToRequest;
    this->gridDisplayProducer->hasGoToRequest = false;
    if (goToRequest != this->patternIdx) {
      this->goToPattern(goToRequest);
    }
  }
  if (this->wait) {
    return;
  }
  if (inputs[SEQ_INPUT].isConnected() && seqInputTrigger.process(inputs[SEQ_INPUT].getVoltage())) {
    if (patternIdx != pattern.goTo) {
      goToPattern(pattern.goTo);
      return;
    }
  }
  if (inputs[GOTO_INPUT].isConnected()) {

    bool ptrnInputIsConnected = inputs[PTRN_INPUT].isConnected();
    unsigned int target = ptrnInputIsConnected ? ((int) std::floor(inputs[PTRN_INPUT].getVoltage() * 12.f)) % NUM_PATTERNS : 0;
    if (goToInputTrigger.process(inputs[GOTO_INPUT].getVoltage())) {
      if (ptrnInputIsConnected) {
        if (target != this->patternIdx) {
          this->goToPattern(target);
          this->lastGoToRequest = target;
          return;
        }
      } else {
        this->goToFirstNonEmpty();
      }
    } else if (inputs[GOTO_INPUT].getVoltage() > 1.0f) {
      if (target != this->lastGoToRequest && target != this->patternIdx) {
        this->goToPattern(target);
        this->lastGoToRequest = target;
        return;
      }
    }
  }
  if (inputs[PREV_INPUT].isConnected() && prevPtrnInputTrigger.process(inputs[PREV_INPUT].getVoltage())) {
    for (int i = this->patternIdx - 1; i >= 0; i--) {
      if (patterns[i].hasCustomSteps()) {
        goToPattern(i);
        return;
      }
    }
    for (unsigned int i = NUM_PATTERNS - 1; i > patternIdx; i--) {
      if (patterns[i].hasCustomSteps()) {
        goToPattern(i);
        return;
      }
    }
  }
  if (inputs[NEXT_INPUT].isConnected() && nextPtrnInputTrigger.process(inputs[NEXT_INPUT].getVoltage())) {
    for (unsigned int i = this->patternIdx + 1; i < NUM_PATTERNS; i++) {
      if (patterns[i].hasCustomSteps()) {
        goToPattern(i);
        return;
      }
    }
    for (unsigned int i = 0; i < this->patternIdx; i++) {
      if (patterns[i].hasCustomSteps()) {
        goToPattern(i);
        return;
      }
    }
  }
  if (inputs[RND_INPUT].isConnected() && firstInputTrigger.process(inputs[RND_INPUT].getVoltage())) {
    unsigned int nonEmpty[NUM_PATTERNS];
    unsigned int idx = 0;
    for (unsigned int i = 0; i < NUM_PATTERNS; i++) {
      if (i != this->patternIdx && patterns[i].hasCustomSteps()) {
        nonEmpty[idx] = i;
        idx++;
      }
    }
    if (idx != 0) {
      unsigned int randIdx = clamp((unsigned int) (random::uniform() * idx), 0, idx);
      goToPattern(nonEmpty[randIdx]);
      return;
    }
  }
  if (inputs[LEFT_INPUT].isConnected() && leftInputTrigger.process(inputs[LEFT_INPUT].getVoltage())) {
    Limits limits = getRowLimits(this->patternIdx);
    for (int i = ((int) patternIdx) - 1; i >= (int) limits.low; i--) {
      if (patterns[i].hasCustomSteps()) {
        goToPattern(i);
        return;
      }
    }
    for (unsigned int i = limits.high - 1; i > patternIdx; i--) {
      if (patterns[i].hasCustomSteps()) {
        goToPattern(i);
        return;
      }
    }
  }
  if (inputs[RIGHT_INPUT].isConnected() && rightInputTrigger.process(inputs[RIGHT_INPUT].getVoltage())) {
    Limits limits = getRowLimits(this->patternIdx);
    for (unsigned int i = patternIdx + 1; i < limits.high; i++) {
      if (patterns[i].hasCustomSteps()) {
        goToPattern(i);
        return;
      }
    }
    for (unsigned int i = limits.low; i < this->patternIdx; i++) {
      if (patterns[i].hasCustomSteps()) {
        goToPattern(i);
        return;
      }
    }
  }
  if (inputs[DOWN_INPUT].isConnected() && downInputTrigger.process(inputs[DOWN_INPUT].getVoltage())) {
    Limits limits = getColumnLimits(patternIdx);
    for (int i = patternIdx - 4; i >= (int) limits.low; i -= 4) {
      if (patterns[i].hasCustomSteps()) {
        goToPattern(i);
        return;
      }
    }
    for (int i = limits.high - 4; i > (int) this->patternIdx; i -= 4) {
      if (patterns[i].hasCustomSteps()) {
        goToPattern(i);
        return;
      }
    }
  }
  if (inputs[UP_INPUT].isConnected() && upInputTrigger.process(inputs[UP_INPUT].getVoltage())) {
    Limits limits = getColumnLimits(patternIdx);
    for (int i = patternIdx + 4; i <= (int) limits.high; i += 4) {
      if (patterns[i].hasCustomSteps()) {
        goToPattern(i);
        return;
      }
    }
    for (unsigned int i = limits.low; i < patternIdx; i += 4) {
      if (patterns[i].hasCustomSteps()) {
        goToPattern(i);
        return;
      }
    }
  }
}

void Phaseque::processButtons() {
  if (tempoTrackButtonTrigger.process(params[TEMPO_TRACK_SWITCH_PARAM].getValue())) {
    tempoTrack ^= true;
    lights[TEMPO_TRACK_LED].value = tempoTrack ? 1.0f : 0.0f;
  }
  if (absModeTrigger.process(params[ABS_MODE_SWITCH_PARAM].getValue())) {
    absMode ^= true;
    lights[ABS_MODE_LED].value = absMode ? 1.0f : 0.0f;
  }
  if (clutchButtonTrigger.process(params[CLUTCH_SWITCH_PARAM].getValue()) || (inputs[CLUTCH_INPUT].isConnected() && clutchInputTrigger.process(inputs[CLUTCH_INPUT].getVoltage()))) {
    clutch ^= true;
    lights[CLUTCH_LED].value = clutch ? 1.0f : 0.0f;
  }
  resetPulse = resetButtonTrigger.process(params[RESET_SWITCH_PARAM].getValue()) || (inputs[RESET_INPUT].isConnected() && resetInputTrigger.process(inputs[RESET_INPUT].getVoltage()));
  if (resetPulse) {
    samplesSinceLastReset = 0;
    tempoTracker.reset();
  } else {
    samplesSinceLastReset++;
    if (samplesSinceLastReset == __INT_MAX__) {
      samplesSinceLastReset = 384000;
    }
  }
  if (resetPulse) {
    lights[RESET_LED].value = 1.1f;
  }
  for (int i = 0; i < NUM_STEPS; i++) {
    if (gateButtonsTriggers[i].process(params[GATE_SWITCH_PARAM + i].getValue())) {
      pattern.steps[i].gate ^= true;
    }
  }
  if (globalGateButtonTrigger.process(params[GLOBAL_GATE_SWITCH_PARAM].getValue())) {
    globalGateInternal ^= true;
  }
}

void Phaseque::processMutaInputs() {
  if (inputs[MUTA_DEC_INPUT].isConnected()) {
    int mutaDecChannels = inputs[MUTA_DEC_INPUT].getChannels();
    if (mutaDecChannels > 1) {
      for (int i = 0; i < mutaDecChannels; i++) {
        int targetStepIdx = i % NUM_STEPS;
        if (mutRstTrigger[i].process(inputs[MUTA_DEC_INPUT].getVoltage(i))) {
          this->resetStepMutation(targetStepIdx);
        } else if (mutDecTrigger[i].process(inputs[MUTA_DEC_INPUT].getVoltage(i))) {
          this->mutateStep(targetStepIdx, -0.05f);
        }
      }
    } else {
      if (mutRstTrigger[0].process(inputs[MUTA_DEC_INPUT].getVoltage())) {
        this->resetMutation();
      } else if (mutDecTrigger[0].process(inputs[MUTA_DEC_INPUT].getVoltage())) {
        this->mutate(-0.05f);
      }
    }
  }
  if (inputs[MUTA_INC_INPUT].isConnected()) {
    int mutaIncChannels = inputs[MUTA_INC_INPUT].getChannels();
    if (mutaIncChannels > 1) {
      for (int i = 0; i < mutaIncChannels; i++) {
        if (mutIncTrigger[i].process(inputs[MUTA_INC_INPUT].getVoltage(i))) {
          int targetStepIdx = i % NUM_STEPS;
          this->mutateStep(targetStepIdx, 0.1f);
        }
      }
    } else {
      if (mutIncTrigger[0].process(inputs[MUTA_INC_INPUT].getVoltage())) {
        this->mutate(0.1f);
      }
    }
  }
}

void Phaseque::processPatternButtons() {
  if (qntTrigger.process(params[QNT_SWITCH_PARAM].getValue())) {
    lights[QNT_LED].value = 1.1f;
    pattern.quantize();
    renderParamQuantities();
    return;
  }
  if (shiftLeftTrigger.process(params[SHIFT_LEFT_SWITCH_PARAM].getValue())) {
    lights[SHIFT_LEFT_LED].value = 1.1f;
    pattern.shiftLeft();
    renderParamQuantities();
    return;
  }
  if (shiftRightTrigger.process(params[SHIFT_RIGHT_SWITCH_PARAM].getValue())) {
    lights[SHIFT_RIGHT_LED].value = 1.1f;
    pattern.shiftRight();
    renderParamQuantities();
    return;
  }
  if (lenTrigger.process(params[LEN_SWITCH_PARAM].getValue())) {
    lights[LEN_LED].value = 1.1f;
    pattern.resetLenghts();
    renderParamQuantities();
    return;
  }
  if (revTrigger.process(params[REV_SWITCH_PARAM].getValue())) {
    lights[REV_LED].value = 1.1f;
    pattern.reverse();
    renderParamQuantities();
    return;
  }
  if (flipTrigger.process(params[FLIP_SWITCH_PARAM].getValue())) {
    lights[FLIP_LED].value = 1.1f;
    pattern.flip();
    renderParamQuantities();
    return;
  }
}

void Phaseque::processJumpInputs() {
  this->jump = false;
  if (absMode || samplesSinceLastReset < 20) {
    return;
  }
  for (int i = 0; i < NUM_STEPS; i++) {
    if (inputs[STEP_JUMP_INPUT + i].isConnected() && jumpInputsTriggers[i].process(inputs[STEP_JUMP_INPUT + i].getVoltage())) {
      jumpToStep(pattern.steps[i]);
      retrigGapGenerator.trigger(1e-4f);
      return;
    }
  }
  if (inputs[RND_JUMP_INPUT].isConnected() && rndJumpInputTrigger.process(inputs[RND_JUMP_INPUT].getVoltage())) {
    int nonMuted[NUM_STEPS];
    int idx = 0;
    for (int i = 0; i < NUM_STEPS; i++) {
      if (!pattern.steps[i].gate ^ !globalGate) { continue; }
      nonMuted[idx] = i;
      idx++;
    }
    if (idx > 0) {
      jumpToStep(pattern.steps[nonMuted[(int) (random::uniform() * idx)]]);
      retrigGapGenerator.trigger(1e-4f);
    }
  }
}

void Phaseque::processIndicators() {
  if (!lightDivider.process()) {
    return;
  }

  lights[GLOBAL_GATE_LED].setBrightness(globalGate);
  lights[GATE_LIGHT].setBrightness(outputs[GATE_OUTPUT].getVoltageSum() / 10.f);

  float v = outputs[V_OUTPUT].getVoltageSum();
  lights[V_POS_LIGHT].setBrightness(std::max(v * 0.5f, 0.0f));
  lights[V_NEG_LIGHT].setBrightness(std::max(v * -0.5f, 0.0f));

  float shift = outputs[SHIFT_OUTPUT].getVoltageSum() / 5.f;
  lights[SHIFT_POS_LIGHT].setBrightness(std::max(shift, 0.0f));
  lights[SHIFT_NEG_LIGHT].setBrightness(std::max(-shift, 0.0f));

  float len = outputs[LEN_OUTPUT].getVoltageSum() / 5.f;
  lights[LEN_POS_LIGHT].setBrightness(std::max(len, 0.0f));
  lights[LEN_NEG_LIGHT].setBrightness(std::max(-len, 0.0f));

  float expr = outputs[EXPR_OUTPUT].getVoltageSum() / 5.f;
  lights[EXPR_POS_LIGHT].setBrightness(std::max(expr, 0.0f));
  lights[EXPR_NEG_LIGHT].setBrightness(std::max(-expr, 0.0f));

  float curve = outputs[EXPR_CURVE_OUTPUT].getVoltageSum() / 5.f;
  lights[EXPR_CURVE_POS_LIGHT].setBrightness(std::max(curve, 0.0f));
  lights[EXPR_CURVE_NEG_LIGHT].setBrightness(std::max(-curve, 0.0f));

  float stepPhase = outputs[PHASE_OUTPUT].getVoltageSum() / 5.f;
  lights[PHASE_LIGHT].setBrightness(stepPhase);
}

void Phaseque::triggerIfBetween(float from, float to) {
  if (!activeStep) {
    return;
  }
  for (int i = 0; i < NUM_STEPS; i++) {
    if (activeStep->idx != i) { continue; }
    if (!pattern.steps[i].gate ^ !globalGate) { continue; }
    float retrigWrapped = fastmod(direction == 1 ? pattern.steps[i].in() : pattern.steps[i].out(), 1.0);
    if (from <= retrigWrapped && retrigWrapped < to) {
      retrigGapGenerator.trigger(1e-4f);
      break;
    }
  }
}

void Phaseque::renderStep(Step *step, int channel) {
  float v = step->attrs[STEP_VALUE].value;
  float shift = step->attrs[STEP_SHIFT].value / baseStepLen;
  float len = step->attrs[STEP_LEN].value / baseStepLen - 1.0f;
  float stepPhase = step->phase(phaseShifted);
  float expr = step->expr(stepPhase);
  float curve = step->attrs[STEP_EXPR_CURVE].value;

  outputs[V_OUTPUT].setVoltage(v, channel);
  outputs[SHIFT_OUTPUT].setVoltage(shift * 5.0f, channel);
  outputs[LEN_OUTPUT].setVoltage(len * 5.0f, channel);
  outputs[EXPR_OUTPUT].setVoltage(expr * 5.0f, channel);
  outputs[EXPR_CURVE_OUTPUT].setVoltage(curve * 5.0f, channel);
  outputs[PHASE_OUTPUT].setVoltage(stepPhase * 10.0f, channel);
}

void Phaseque::renderUnison(Step *step, int channel) {
  float v = step->attrs[STEP_VALUE].base;
  float shift = step->attrs[STEP_SHIFT].base / baseStepLen;
  float len = step->attrs[STEP_LEN].base / baseStepLen - 1.0f;
  float stepPhase = step->phaseBase(phaseShifted);
  float expr = step->exprBase(stepPhase);
  float curve = step->attrs[STEP_EXPR_CURVE].base;

  outputs[V_OUTPUT].setVoltage(v, channel);
  outputs[SHIFT_OUTPUT].setVoltage(shift * 5.0f, channel);
  outputs[LEN_OUTPUT].setVoltage(len * 5.0f, channel);
  outputs[EXPR_OUTPUT].setVoltage(expr * 5.0f, channel);
  outputs[EXPR_CURVE_OUTPUT].setVoltage(curve * 5.0f, channel);
  outputs[PHASE_OUTPUT].setVoltage(stepPhase * 10.0f, channel);
}

void Phaseque::process(const ProcessArgs &args) {
  processGlobalParams();
  processPatternNav();
  processMutaInputs();
  if (this->buttonsDivider.process()) {
    processButtons();
    processPatternButtons();
  }
  return;

  // Phase param
  float phaseParamInput = params[PHASE_PARAM].getValue();
  bool phaseWasZeroed = false;
  if (phaseParamInput != phaseParam) {
    if (phaseParamInput == 0.0f || (lastPhaseParamInput < 0.1f && phaseParamInput > 0.9f) || (lastPhaseParamInput > 0.9f && phaseParamInput < 0.1f)) {
      if (phaseParamInput == 0.0f) {
        phaseWasZeroed = true;
      }
      phaseParam = phaseParamInput;
    } else {
      float delta = phaseParamInput - phaseParam;
      phaseParam = phaseParam + delta * args.sampleTime * 50.0f; // Smoothing
    }
  }

  if (absMode) {
    resolution = 1.0;
  } else {
    resolution = pattern.resolution;
  }

  processJumpInputs();

  if (resetPulse) {
    phase = 0.0;
    phaseWasZeroed = true;
  }

  if (inputs[PHASE_INPUT].isConnected()) {
    bpmDisabled = true;
    double phaseIn = inputs[PHASE_INPUT].getVoltage() / 10.0;
    while (phaseIn >= 1.0) {
      phaseIn = phaseIn - 1.0;
    }
    while (phaseIn < 0.0) {
      phaseIn = phaseIn + 1.0;
    }
    double phaseInDelta = 0.0;
    if (resetPulse) {
      phaseInDelta = lastPhaseInDelta;
    } else {
      if (lastPhaseInState) {
        // Trust the state
        phaseInDelta = phaseIn - lastPhaseIn;
        if (fabs(phaseInDelta) > 0.01) {
          if (samplesSinceLastReset < 20) {
            // Nah! It's a cable-delay anomaly.
            phaseInDelta = lastPhaseInDelta;
          } else if (phaseIn < 0.01 || phaseIn > 0.99) {
            // Handle phase-flip
            if (phaseIn < lastPhaseIn) {
              phaseInDelta = 1.0f + phaseInDelta;
            }
            if (phaseIn > lastPhaseIn) {
              phaseInDelta = phaseInDelta - 1.0f;
            }
          }
        }
      }
    }
    if (clutch) {
      if (absMode) {
        phase = phaseIn;
      } else {
        if (inputs[CLOCK_INPUT].isConnected() && clockTrigger.process(inputs[CLOCK_INPUT].getVoltage())) {
          float targetPhase = phase + phaseInDelta / resolution;
          float delta = fmodf(targetPhase * resolution, 1.0f);
          if (delta < 0.01) {
            phase = roundf(targetPhase * resolution) / resolution;
          } else {
            phase = targetPhase;
          }
        } else {
          phase = phase + phaseInDelta / resolution;
        }
      }
    }
    lastPhaseIn = phaseIn;
    lastPhaseInDelta = phaseInDelta;
  } else if (inputs[CLOCK_INPUT].isConnected()) {
    bpmDisabled = false;
    if (!lastClockInputState || lastPhaseInState) {
      tempoTracker.reset();
    }
    float currentStep = floorf(phase * resolution);
    float sampleTime = args.sampleTime;
    if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage()) && samplesSinceLastReset > 19) {
      tempoTracker.tick(sampleTime);
      if (clutch) {
        if (bps < 0.0f) {
          float nextStep = eucMod(currentStep, resolution);
          float nextPhase = fastmod(nextStep / resolution, 1.0f);
          phase = nextPhase;
        } else {
          float nextStep = eucMod(currentStep + 1.0f, resolution);
          float nextPhase = fastmod(nextStep / resolution, 1.0f);
          phase = nextPhase;
        }
      }
      tickedAtLastSample = true;
    } else {
      tempoTracker.acc(sampleTime);
      if (inputs[VBPS_INPUT].isConnected()) {
        bps = inputs[VBPS_INPUT].getVoltage();
      } else if (tempoTrack) {
        if (tempoTracker.detected) {
          bps = tempoTracker.bps;
        }
      } else {
        bps = params[BPM_PARAM].getValue() / 60.0f;
      }
      float nextPhase = fastmod(phase + bps * args.sampleTime / resolution, 1.0f);
      float nextStep = floorf(nextPhase * resolution);
      if (clutch) {
        if (nextStep == currentStep || (bps < 0.0f && (tickedAtLastSample || resetPulse))) {
          phase = nextPhase;
        }
      }
      tickedAtLastSample = false;
    }
  } else if (inputs[VBPS_INPUT].isConnected()) {
    bpmDisabled = false;
    bps = inputs[VBPS_INPUT].getVoltage();
    if (clutch) {
      float nextPhase = fastmod(phase + bps * args.sampleTime / resolution, 1.0f);
      phase = nextPhase;
    }
  }
  float preciseBpm = bps * 60.0f;
  float roundedBpm = roundf(preciseBpm);
  if (isNear(preciseBpm, roundedBpm, 0.06f)) {
    bpm = roundedBpm;
  } else {
    bpm = preciseBpm;
  }
  if (std::isnan(phase)) {
    phase = 0.0;
  }
  while (phase >= 1.0) {
    phase = phase - 1.0;
  }
  while (phase < 0.0) {
    phase = phase + 1.0;
  }
  phaseShifted = phase + phaseParam;
  while (phaseShifted >= 1.0f) {
    phaseShifted = phaseShifted - 1.0f;
  }
  while (phaseShifted < 0.0f) {
    phaseShifted = phaseShifted + 1.0f;
  }

  float phaseDelta = phaseShifted - lastPhaseShifted;
  if (phaseDelta != 0.0f && fabsf(phaseDelta) < 0.95f && !phaseWasZeroed && !jump) {
    direction = phaseDelta > 0.0f ? 1 : -1;
  }
  if (phaseShifted < 0.05f && lastPhaseShifted > 0.95f) {
    ptrnEndPulseGenerator.trigger(1e-3f);
  } else if (phaseShifted > 0.95f && lastPhaseShifted < 0.05f) {
    ptrnStartPulseGenerator.trigger(1e-3f);
  }

  if (polyphonyMode == POLYPHONIC) {
    activeStep = nullptr;
    pattern.updateStepsStates(phaseShifted, globalGate, stepsStates, false);
  } else if (polyphonyMode == UNISON) {
    activeStep = nullptr;
    pattern.updateStepsStates(phaseShifted, globalGate, stepsStates, false);
    pattern.updateStepsStates(phaseShifted, globalGate, unisonStates, true);
  } else {
    activeStep = pattern.getStepForPhase(phaseShifted, globalGate);
    for (int i = 0; i < NUM_STEPS; i++) {
      stepsStates[i] = false;
      unisonStates[i] = false;
    }
  }
  if ((activeStep && lastActiveStep) && activeStep != lastActiveStep) {
    retrigGapGenerator.trigger(1e-4f);
  }
  lastActiveStep = activeStep;

  outputs[PTRN_START_OUTPUT].setVoltage(ptrnStartPulseGenerator.process(args.sampleTime) ? 10.0f : 0.0f);
  outputs[PTRN_END_OUTPUT].setVoltage(ptrnEndPulseGenerator.process(args.sampleTime) ? 10.0f : 0.0f);
  outputs[PTRN_WRAP_OUTPUT].setVoltage(std::max(outputs[PTRN_START_OUTPUT].value, outputs[PTRN_END_OUTPUT].value));

  float voltsForPattern = (patternIdx - 1) * 1.0f / 12.0f;
  outputs[PTRN_OUTPUT].setVoltage(voltsForPattern);

  if (patternIdx != lastPatternIdx) {
    wentPulseGenerator.trigger(1e-3f);
    lastPatternIdx = patternIdx;
  }
  outputs[WENT_OUTPUT].setVoltage(wentPulseGenerator.process(args.sampleTime) ? 10.0f : 0.0f);

  if (resetPulse && !absMode) {
    retrigGapGenerator.trigger(1e-4f);
  }

  retrigGap = retrigGapGenerator.process(args.sampleTime);
  if (polyphonyMode == MONOPHONIC) {
    if (retrigGap || !clutch) {
      outputs[GATE_OUTPUT].setVoltage(0.0f);
    } else {
      outputs[GATE_OUTPUT].setVoltage(activeStep ? 10.0f : 0.0f);
    }
  }
  if (polyphonyMode == POLYPHONIC || polyphonyMode == UNISON) {
    for (int i = 0; i < NUM_STEPS; i++) {
      if (stepsStates[i]) {
        renderStep(&pattern.steps[i], i);
        outputs[GATE_OUTPUT].setVoltage(clutch ? 10.f : 0.f, i);
        outputs[STEP_GATE_OUTPUT + i].setVoltage(10.f);
        lights[STEP_GATE_LIGHT + i].setBrightness(1.f);
      } else {
        outputs[GATE_OUTPUT].setVoltage(0.f, i);
        outputs[STEP_GATE_OUTPUT + i].setVoltage(0.f);
        lights[STEP_GATE_LIGHT + i].setBrightness(0.f);
      }
      if (polyphonyMode == UNISON) {
        if (unisonStates[i]) {
          renderUnison(&pattern.steps[i], i + NUM_STEPS);
          outputs[GATE_OUTPUT].setVoltage(clutch ? 10.f : 0.f, i + NUM_STEPS);
          outputs[STEP_GATE_OUTPUT + i].setVoltage(10.f);
          lights[STEP_GATE_LIGHT + i].setBrightness(1.f);
        } else {
          outputs[GATE_OUTPUT].setVoltage(0.f, i + NUM_STEPS);
        }
      }
    }
    if (polyphonyMode == UNISON) {
      outputs[GATE_OUTPUT].setChannels(NUM_STEPS * 2);
      outputs[V_OUTPUT].setChannels(NUM_STEPS * 2);
      outputs[SHIFT_OUTPUT].setChannels(NUM_STEPS * 2);
      outputs[LEN_OUTPUT].setChannels(NUM_STEPS * 2);
      outputs[EXPR_OUTPUT].setChannels(NUM_STEPS * 2);
      outputs[EXPR_CURVE_OUTPUT].setChannels(NUM_STEPS * 2);
      outputs[PHASE_OUTPUT].setChannels(NUM_STEPS * 2);
    } else {
      outputs[GATE_OUTPUT].setChannels(NUM_STEPS);
      outputs[V_OUTPUT].setChannels(NUM_STEPS);
      outputs[SHIFT_OUTPUT].setChannels(NUM_STEPS);
      outputs[LEN_OUTPUT].setChannels(NUM_STEPS);
      outputs[EXPR_OUTPUT].setChannels(NUM_STEPS);
      outputs[EXPR_CURVE_OUTPUT].setChannels(NUM_STEPS);
      outputs[PHASE_OUTPUT].setChannels(NUM_STEPS);
    }

    processIndicators();
  } else {
    if (activeStep) {
      renderStep(activeStep, 0);
      outputs[GATE_OUTPUT].setChannels(1);
      outputs[V_OUTPUT].setChannels(1);
      outputs[SHIFT_OUTPUT].setChannels(1);
      outputs[LEN_OUTPUT].setChannels(1);
      outputs[EXPR_OUTPUT].setChannels(1);
      outputs[EXPR_CURVE_OUTPUT].setChannels(1);
      outputs[PHASE_OUTPUT].setChannels(1);

      processIndicators();

      for (int i = 0; i < NUM_STEPS; i++) {
        outputs[STEP_GATE_OUTPUT + i].setVoltage(activeStep->idx == i ? 10.0f : 0.0f);
        lights[STEP_GATE_LIGHT + i].setBrightness(activeStep->idx == i ? 1.0f : 0.0f);
      }
    } else {
      for (int i = 0; i < NUM_STEPS; i++) {
        outputs[STEP_GATE_OUTPUT + i].setVoltage(0.0f);
        lights[STEP_GATE_LIGHT + i].setBrightness(0.0f);
      }
    }
  }

  outputs[PTRN_PHASE_OUTPUT].setVoltage(phaseShifted * 10.0f);

  for (int i = 0; i < NUM_STEPS; i++) {
    lights[GATE_SWITCH_LED + i].setBrightness(pattern.steps[i].gate ^ !globalGate);
  }

  lights[PHASE_LED].setBrightness(inputs[PHASE_INPUT].isConnected());
  lights[CLOCK_LED].setBrightness(inputs[CLOCK_INPUT].isConnected());
  lights[VBPS_LED].setBrightness(inputs[VBPS_INPUT].isConnected() && !inputs[PHASE_INPUT].isConnected());

  lastPhase = phase;
  lastPhaseInState = inputs[PHASE_INPUT].isConnected();
  lastPhaseShifted = phaseShifted;
  lastPhaseParamInput = params[PHASE_PARAM].getValue();
  lastClockInputState = inputs[CLOCK_INPUT].isConnected();

  if (this->gridDisplayConsumer->consumed && this->gridDisplayConsumer->currentPattern != this->patternIdx) {
    this->gridDisplayConsumer->currentPattern = this->patternIdx;
    this->gridDisplayConsumer->currentPatternGoTo = this->pattern.goTo;
    this->gridDisplayConsumer->consumed = false;
    for (unsigned int i = 0; i < NUM_PATTERNS; i++) {
      this->gridDisplayConsumer->dirtyMask.set(i, this->patterns[i].hasCustomSteps());
    }
  }
  if (this->mainDisplayConsumer->consumed) {
    this->mainDisplayConsumer->resolution = this->pattern.resolution;
    this->mainDisplayConsumer->phase = this->phaseShifted;
    this->mainDisplayConsumer->direction = this->direction;
    this->mainDisplayConsumer->pattern = this->pattern;
    this->mainDisplayConsumer->globalGate = this->globalGate;
    this->mainDisplayConsumer->polyphonyMode = this->polyphonyMode;
    std::memcpy(this->mainDisplayConsumer->stepsStates, this->stepsStates, sizeof(this->stepsStates));
    std::memcpy(this->mainDisplayConsumer->unisonStates, this->unisonStates, sizeof(this->unisonStates));
    this->mainDisplayConsumer->globalShift = this->globalShift;
    this->mainDisplayConsumer->globalLen = this->globalLen;
    if (this->activeStep) {
      this->mainDisplayConsumer->activeStep = *this->activeStep;
      this->mainDisplayConsumer->hasActiveStep = true;
    } else {
      this->mainDisplayConsumer->hasActiveStep = false;
    }
    this->mainDisplayConsumer->exprCurveCV = inputs[GLOBAL_EXPR_CURVE_INPUT].getVoltage();
    this->mainDisplayConsumer->exprPowerCV = inputs[GLOBAL_EXPR_POWER_INPUT].getVoltage();
    this->mainDisplayConsumer->consumed = false;
  }
}

Model *modelPhaseque = createModel<Phaseque, PhasequeWidget>("Phaseque");
