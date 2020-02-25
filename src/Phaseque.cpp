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
    unsigned int target = ptrnInputIsConnected ? eucMod((int) std::floor(inputs[PTRN_INPUT].getVoltage() * 12.f), NUM_PATTERNS) : 0;
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
  if (waitButtonTrigger.process(params[WAIT_SWITCH_PARAM].getValue())) {
    this->wait ^= true;
  }
  if (tempoTrackButtonTrigger.process(params[TEMPO_TRACK_SWITCH_PARAM].getValue())) {
    this->tempoTrack ^= true;
    lights[TEMPO_TRACK_LED].value = tempoTrack ? 1.0f : 0.0f;
  }
  if (absModeTrigger.process(params[ABS_MODE_SWITCH_PARAM].getValue())) {
    this->absMode ^= true;
    lights[ABS_MODE_LED].value = absMode ? 1.0f : 0.0f;
  }
  for (unsigned int stepIdx = 0; stepIdx < this->pattern.size; stepIdx++) {
    if (gateButtonsTriggers[stepIdx].process(params[GATE_SWITCH_PARAM + stepIdx].getValue())) {
      unsigned int blockIdx = stepIdx / 4;
      unsigned int stepInBlockIdx = stepIdx % 4;
      int intMask = 1 << stepInBlockIdx;
      simd::float_4 mask = simd::movemaskInverse<simd::float_4>(intMask);
      this->pattern.stepGates[blockIdx] ^= mask;
    }
  }
  if (globalGateButtonTrigger.process(params[GLOBAL_GATE_SWITCH_PARAM].getValue())) {
    this->globalGateInternal ^= true;
  }
}

void Phaseque::processClutchAndReset() {
  if (clutchButtonTrigger.process(params[CLUTCH_SWITCH_PARAM].getValue()) || (inputs[CLUTCH_INPUT].isConnected() && clutchInputTrigger.process(inputs[CLUTCH_INPUT].getVoltage()))) {
    clutch ^= true;
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
  // TODO: Implement this
  // this->jump = false;
  // if (absMode || samplesSinceLastReset < 20) {
  //   return;
  // }
  // for (int i = 0; i < NUM_STEPS; i++) {
  //   if (inputs[STEP_JUMP_INPUT + i].isConnected() && jumpInputsTriggers[i].process(inputs[STEP_JUMP_INPUT + i].getVoltage())) {
  //     jumpToStep(pattern.steps[i]);
  //     retrigGapGenerator.trigger(1e-4f);
  //     return;
  //   }
  // }
  // if (inputs[RND_JUMP_INPUT].isConnected() && rndJumpInputTrigger.process(inputs[RND_JUMP_INPUT].getVoltage())) {
  //   int nonMuted[NUM_STEPS];
  //   int idx = 0;
  //   for (int i = 0; i < NUM_STEPS; i++) {
  //     if (!pattern.steps[i].gate ^ !globalGate) { continue; }
  //     nonMuted[idx] = i;
  //     idx++;
  //   }
  //   if (idx > 0) {
  //     jumpToStep(pattern.steps[nonMuted[(int) (random::uniform() * idx)]]);
  //     retrigGapGenerator.trigger(1e-4f);
  //   }
  // }
}

void Phaseque::processIndicators() {
  if (!lightDivider.process()) {
    return;
  }

  for (unsigned int stepIdx = 0; stepIdx < this->pattern.size; stepIdx++) {
    unsigned int blockIdx = stepIdx / 4;
    unsigned int stepInBlockIdx = stepIdx % 4;

    if (this->polyphonyMode == PolyphonyModes::MONOPHONIC) {
      lights[STEP_GATE_LIGHT + stepIdx].setBrightness(
        this->pattern.hasActiveStep && (this->pattern.activeStepIdx == stepIdx) ? 1.f : 0.f
      );
    } else {
      lights[STEP_GATE_LIGHT + stepIdx].setBrightness(
        simd::movemask(this->pattern.hitsTemp[blockIdx]) & 1 << stepInBlockIdx ? 1.f : 0.f
      );
    }

    lights[GATE_SWITCH_LED + stepIdx].setBrightness((simd::movemask(this->pattern.stepGates[blockIdx]) & (1 << stepInBlockIdx)) ^ !globalGate);
  }

  if (this->wait) {
    lights[WAIT_LED].value = 1.1f;
  }

  lights[CLUTCH_LED].value = this->clutch ? 1.f : 0.f;

  lights[PHASE_LED].setBrightness(inputs[PHASE_INPUT].isConnected());
  lights[CLOCK_LED].setBrightness(inputs[CLOCK_INPUT].isConnected());
  lights[VBPS_LED].setBrightness(inputs[VBPS_INPUT].isConnected() && !inputs[PHASE_INPUT].isConnected());

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

simd::float_4 getBlockExpressions(
  simd::float_4 exprIn,
  simd::float_4 exprOut,
  simd::float_4 exprPower,
  simd::float_4 exprCurve,
  simd::float_4 phase
) {
  simd::float_4 isRising = exprOut > exprIn;
  simd::float_4 curveBendedUp = exprCurve > 0.f;
  simd::float_4 invertPower = ~(isRising ^ curveBendedUp);
  simd::float_4 finalPhase = simd::ifelse(invertPower, 1.f - phase, phase);
  // Our target is between x^2 and x^8
  simd::float_4 finalPower = 5.f + simd::ifelse(invertPower, -exprPower, exprPower) * 3.f;
  simd::float_4 powOutput = simd::pow(finalPhase, finalPower);
  simd::float_4 exprResult = simd::ifelse(invertPower, 1.f - powOutput, powOutput);
  simd::float_4 exprMix = simd::crossfade(phase, exprResult, simd::abs(exprCurve));
  simd::float_4 exprAmplitude = exprOut - exprIn;
  simd::float_4 exprScaled = exprIn + exprMix * exprAmplitude;
  return exprScaled;
}

void Phaseque::renderStepMono() {
  unsigned int stepInBlockIdx = this->pattern.activeStepInBlockIdx;
  float v[4];
  this->pattern.stepBases[StepAttr::STEP_VALUE][this->pattern.activeBlockIdx].store(v);
  float shift[4];
  this->pattern.stepBases[StepAttr::STEP_SHIFT][this->pattern.activeBlockIdx].store(shift);
  float len[4];
  this->pattern.stepBases[StepAttr::STEP_LEN][this->pattern.activeBlockIdx].store(len);

  // Calculating the step phase
  simd::float_4 patternPhase(phaseShifted);
  simd::float_4 stepIns = this->pattern.stepInsComputed[this->pattern.activeBlockIdx];
  simd::float_4 stepPhases = (patternPhase - simd::ifelse(patternPhase < stepIns, stepIns - 1.f, stepIns)) / this->pattern.stepBases[StepAttr::STEP_LEN][this->pattern.activeBlockIdx];

  float stepPhasesScalar[4];
  stepPhases.store(stepPhasesScalar);

  float curve[4];
  this->pattern.stepBases[StepAttr::STEP_EXPR_CURVE][this->pattern.activeBlockIdx].store(curve);

  float expressions[4];
  getBlockExpressions(
    this->pattern.stepBases[StepAttr::STEP_EXPR_IN][this->pattern.activeBlockIdx],
    this->pattern.stepBases[StepAttr::STEP_EXPR_OUT][this->pattern.activeBlockIdx],
    this->pattern.stepBases[StepAttr::STEP_EXPR_POWER][this->pattern.activeBlockIdx],
    this->pattern.stepBases[StepAttr::STEP_EXPR_CURVE][this->pattern.activeBlockIdx],
    stepPhases
  ).store(expressions);

  outputs[V_OUTPUT].setVoltage(v[stepInBlockIdx]);
  outputs[SHIFT_OUTPUT].setVoltage(shift[stepInBlockIdx] / this->pattern.baseStepLen * 5.f);
  outputs[LEN_OUTPUT].setVoltage((len[stepInBlockIdx] / this->pattern.baseStepLen - 1.f) * 5.f);
  outputs[EXPR_OUTPUT].setVoltage(expressions[stepInBlockIdx] * 5.f);
  outputs[EXPR_CURVE_OUTPUT].setVoltage(curve[stepInBlockIdx] * 5.f);
  outputs[PHASE_OUTPUT].setVoltage(stepPhases[stepInBlockIdx] * 10.f);
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

bool Phaseque::processPhaseParam(float sampleTime) {
  float phaseParamInput = params[PHASE_PARAM].getValue();
  if (phaseParamInput != phaseParam) {
    bool flipped = std::abs(phaseParamInput - lastPhaseParamInput) > 0.9;
    if (phaseParamInput == 0.f || flipped) {
      phaseParam = phaseParamInput; // No smoothing
    } else {
      float delta = phaseParamInput - phaseParam;
      phaseParam = phaseParam + delta * sampleTime * 50.0f; // Smoothing
    }
  }
  return phaseParamInput == 0.f;
}

void Phaseque::processTransport(bool phaseWasZeroed, float sampleTime) {
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
      float nextPhase = fastmod(phase + bps * sampleTime / resolution, 1.0f);
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
      float nextPhase = fastmod(phase + bps * sampleTime / resolution, 1.0f);
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
}

void Phaseque::feedDisplays() {
  if (this->gridDisplayConsumer->consumed) {
    this->gridDisplayConsumer->currentPattern = this->patternIdx;
    this->gridDisplayConsumer->currentPatternGoTo = this->pattern.goTo;
    this->gridDisplayConsumer->consumed = false;
    for (unsigned int i = 0; i < NUM_PATTERNS; i++) {
      this->gridDisplayConsumer->dirtyMask.set(i, this->patterns[i].hasCustomSteps());
    }
  }
  if (this->mainDisplayConsumer->consumed) {
    this->mainDisplayConsumer->phase = this->phaseShifted;
    this->mainDisplayConsumer->direction = this->direction;
    this->mainDisplayConsumer->pattern = this->pattern;
    this->mainDisplayConsumer->globalGate = this->globalGate;
    this->mainDisplayConsumer->polyphonyMode = this->polyphonyMode;
    this->mainDisplayConsumer->exprCurveCV = inputs[GLOBAL_EXPR_CURVE_INPUT].getVoltage();
    this->mainDisplayConsumer->exprPowerCV = inputs[GLOBAL_EXPR_POWER_INPUT].getVoltage();
    this->mainDisplayConsumer->consumed = false;
  }
}

void Phaseque::process(const ProcessArgs &args) {
  float sampleTime = args.sampleTime;
  this->processGlobalParams();
  this->processPatternNav();
  this->processMutaInputs();
  if (this->buttonsDivider.process()) {
    this->processButtons();
    this->processPatternButtons();
  }
  this->processClutchAndReset();

  // Phase param
  bool phaseWasZeroed = this->processPhaseParam(sampleTime);

  if (absMode) {
    this->resolution = 1;
    this->resolutionDisplay = 1.f;
  } else {
    this->resolution = pattern.resolution;
    this->resolutionDisplay = pattern.resolution;
  }

  this->processJumpInputs();
  this->processTransport(phaseWasZeroed, sampleTime);

  lastActiveStep = activeStep;

  outputs[PTRN_START_OUTPUT].setVoltage(ptrnStartPulseGenerator.process(sampleTime) ? 10.0f : 0.0f);
  outputs[PTRN_END_OUTPUT].setVoltage(ptrnEndPulseGenerator.process(sampleTime) ? 10.0f : 0.0f);
  outputs[PTRN_WRAP_OUTPUT].setVoltage(std::max(outputs[PTRN_START_OUTPUT].value, outputs[PTRN_END_OUTPUT].value));

  float voltsForPattern = (patternIdx - 1) * 1.0f / 12.0f;
  outputs[PTRN_OUTPUT].setVoltage(voltsForPattern);

  if (patternIdx != lastPatternIdx) {
    wentPulseGenerator.trigger(1e-3f);
    lastPatternIdx = patternIdx;
  }
  outputs[WENT_OUTPUT].setVoltage(wentPulseGenerator.process(sampleTime) ? 10.0f : 0.0f);

  if (resetPulse && !absMode) {
    retrigGapGenerator.trigger(1e-4f);
  }

  outputs[PTRN_PHASE_OUTPUT].setVoltage(phaseShifted * 10.f);

  this->pattern.findStepsForPhase(this->phaseShifted);
  this->pattern.findMonoStep();

  int channels = this->polyphonyMode == PolyphonyModes::MONOPHONIC ? 1 : this->polyphonyMode == PolyphonyModes::POLYPHONIC ? NUM_STEPS : NUM_STEPS * 2;

  if (this->polyphonyMode == PolyphonyModes::MONOPHONIC) {
    for (int i = 0; i < NUM_STEPS; i++) {
      outputs[STEP_GATE_OUTPUT + i].setVoltage(0.f);
    }
    if (this->pattern.hasActiveStep) {
      if (this->pattern.activeStepIdx != this->pattern.lastActiveStepIdx) {
        retrigGapGenerator.trigger(1e-4f);
      }
      this->renderStepMono();
      outputs[STEP_GATE_OUTPUT + this->pattern.activeStepIdx].setVoltage(10.f);
    }
    bool retrigGap = retrigGapGenerator.process(sampleTime);
    outputs[GATE_OUTPUT].setVoltage(this->clutch && this->pattern.hasActiveStep && !retrigGap ? 10.f : 0.f);
  } else {
    for (int i = 0; i < NUM_STEPS; i++) {
      if (stepsStates[i]) {
        // TODO: Enable this
        // renderStep(&pattern.steps[i], i);
        outputs[GATE_OUTPUT].setVoltage(clutch ? 10.f : 0.f, i);
        outputs[STEP_GATE_OUTPUT + i].setVoltage(10.f);
        // lights[STEP_GATE_LIGHT + i].setBrightness(1.f);
      } else {
        outputs[GATE_OUTPUT].setVoltage(0.f, i);
        outputs[STEP_GATE_OUTPUT + i].setVoltage(0.f);
        // lights[STEP_GATE_LIGHT + i].setBrightness(0.f);
      }
      if (polyphonyMode == UNISON) {
        if (unisonStates[i]) {
          // TODO: Enable this
          // renderUnison(&pattern.steps[i], i + NUM_STEPS);
          outputs[GATE_OUTPUT].setVoltage(clutch ? 10.f : 0.f, i + NUM_STEPS);
          outputs[STEP_GATE_OUTPUT + i].setVoltage(10.f);
          // lights[STEP_GATE_LIGHT + i].setBrightness(1.f);
        } else {
          outputs[GATE_OUTPUT].setVoltage(0.f, i + NUM_STEPS);
        }
      }
    }
  }

  outputs[GATE_OUTPUT].setChannels(channels);
  outputs[V_OUTPUT].setChannels(channels);
  outputs[SHIFT_OUTPUT].setChannels(channels);
  outputs[LEN_OUTPUT].setChannels(channels);
  outputs[EXPR_OUTPUT].setChannels(channels);
  outputs[EXPR_CURVE_OUTPUT].setChannels(channels);
  outputs[PHASE_OUTPUT].setChannels(channels);

  this->processIndicators();

  lastPhase = phase;
  lastPhaseInState = inputs[PHASE_INPUT].isConnected();
  lastPhaseShifted = phaseShifted;
  lastPhaseParamInput = params[PHASE_PARAM].getValue();
  lastClockInputState = inputs[CLOCK_INPUT].isConnected();

  this->feedDisplays();
}

Model *modelPhaseque = createModel<Phaseque, PhasequeWidget>("Phaseque");
