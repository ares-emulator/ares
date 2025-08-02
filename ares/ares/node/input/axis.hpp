struct Axis : Input {
  DeclareClass(Axis, "input.axis")
  using Input::Input;

  enum class Response : int {
    Linear,
    RelaxedToAggressive,
    RelaxedToLinear,
    LinearToRelaxed,
    AggressiveToRelaxed,
    AggressiveToLinear,
    LinearToAggressive,
  } response = Response::Linear;

  auto value() const -> s64 { return _value; }
  auto setValue(s64 value) -> void { _value = value; }
  
  auto outputStyle() const -> string { return _outputStyle; }
  auto maxOutputReducerOneFactor() const -> f64 { return _maxOutputReducerOneFactor; }
  auto maxOutputReducerTwoFactor() const -> f64 { return _maxOutputReducerTwoFactor; }
  auto customMaxOutput() const -> f64 { return _customMaxOutput; }
  auto deadzoneShape() const -> string { return _deadzoneShape; }
  auto deadzoneSize() const -> f64 { return _deadzoneSize; }
  auto proportionalSensitivity() const -> f64 { return _proportionalSensitivity; }
  auto responseCurve() const -> string { return _responseCurve; }
  auto rangeNormalizedSwitchDistance() const -> f64 { return _rangeNormalizedSwitchDistance; }
  auto responseStrength() const -> f64 { return _responseStrength; }
  auto virtualNotch() const -> bool { return _virtualNotch; }
  auto notchLengthFromEdge() const -> f64 { return _notchLengthFromEdge; }
  auto notchAngularSnappingDistance() const -> f64 { return _notchAngularSnappingDistance; }
  
  auto setOutputStyle(string outputStyle) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _outputStyle = outputStyle;
  }

  auto setMaxOutputReducerOneFactor(f64 maxOutputReducerOneFactor) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _maxOutputReducerOneFactor = maxOutputReducerOneFactor;
  }

  auto setMaxOutputReducerTwoFactor(f64 maxOutputReducerTwoFactor) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _maxOutputReducerTwoFactor = maxOutputReducerTwoFactor;
  }

  auto setCustomMaxOutput(f64 customMaxOutput) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _customMaxOutput = customMaxOutput;
  }

  auto setDeadzoneShape(string deadzoneShape) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _deadzoneShape = deadzoneShape;
  }

  auto setDeadzoneSize(f64 deadzoneSize) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _deadzoneSize = deadzoneSize;
  }

  auto setProportionalSensitivity(f64 proportionalSensitivity) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _proportionalSensitivity = proportionalSensitivity;
  }

  auto setResponseCurve(string responseCurve) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _responseCurve = responseCurve;
  }

  auto setRangeNormalizedSwitchDistance(f64 rangeNormalizedSwitchDistance) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _rangeNormalizedSwitchDistance = rangeNormalizedSwitchDistance;
  }

  auto setResponseStrength(f64 responseStrength) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _responseStrength = responseStrength;
  }

  auto setVirtualNotch(bool virtualNotch) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _virtualNotch = virtualNotch;
  }

  auto setNotchLengthFromEdge(f64 notchLengthFromEdge) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _notchLengthFromEdge = notchLengthFromEdge;
  }

  auto setNotchAngularSnappingDistance(f64 notchAngularSnappingDistance) -> void {
    lock_guard<recursive_mutex> lock(_mutex);
    _notchAngularSnappingDistance = notchAngularSnappingDistance;
  }

  auto setOperatingRange(s16 position, double saturationRadius, double offset) -> double {
    return position * saturationRadius / 32767.0 + offset;
  }

  auto revisePosition(double position, double length, double offset) -> double {
    return (position - offset) * length + offset;
  }
  
  auto virtualNotch(double initialLength, double initialAngle, double saturationRadius, double configuredNotchLengthFromEdge, double configuredMaxNotchAngularDist) -> double {
    if(configuredNotchLengthFromEdge > 0.0 && configuredMaxNotchAngularDist > 0.0) {
      auto lengthToNotchStart = (1.0 - configuredNotchLengthFromEdge) * saturationRadius;
      double maxNotchAngularDistRadians = configuredMaxNotchAngularDist * Math::Pi / 180.0;
      if(initialLength >= lengthToNotchStart) {
        auto angle = initialAngle + 2.0 * Math::Pi;
        auto windowedAngle = angle;
        while(windowedAngle > Math::Pi / 4.0) windowedAngle -= Math::Pi / 4.0;
        if((windowedAngle <= 0.0 + maxNotchAngularDistRadians) || (windowedAngle >= Math::Pi / 4.0 - maxNotchAngularDistRadians)) {
          angle += maxNotchAngularDistRadians;
          angle -= fmod(angle, Math::Pi / 4.0);
          return angle;
        }
      }
    }
    return initialAngle;
  }

  auto responseCurve(double position, double configuredInnerDeadzone, double saturationRadius, double offset, double configuredRangeNormalizedSwitchDistance, double configuredResponseStrength, double configuredProportionalSensitivity, string configuredResponseCurveMode) -> double {
    auto switchDistance = configuredRangeNormalizedSwitchDistance * (saturationRadius - configuredInnerDeadzone) + configuredInnerDeadzone;
    auto b = 1.0;  //keep for clarity or remove to reduce number of operations performed?
    auto c = b * (log(1.0 - cos(Math::Pi * (switchDistance - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone))) - log(2.0)) / log((switchDistance - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)); //c = 2.0 * log(sin(Math::Pi/2.0*(switchDistance-configuredInnerDeadzone)/(saturationRadius-configuredInnerDeadzone)))/log((switchDistance-configuredInnerDeadzone)/(saturationRadius-configuredInnerDeadzone)); more efficient but requires b = 1.0 to remove a
    auto maxA = 0.0;
    auto a = 0.0;

    if(response == Response::AggressiveToRelaxed || response == Response::AggressiveToLinear || response == Response::LinearToAggressive) {
      maxA = Math::Pi * 1e-09 / (Math::Pi * 1e-09 - c * (saturationRadius - configuredInnerDeadzone) * tan(Math::Pi * 1e-09 / (2 * (saturationRadius - configuredInnerDeadzone)))); //high values of a can cause early input to approach infinity; take derivative of response function and solve for a at point (innerDeadzone + 1e-09, 0) where 1e-09 is an epsilon
      a = configuredResponseStrength * (maxA - 1.0) + 1.0;
    } else {
      maxA = 1.0;
      a = (1.0 - configuredResponseStrength) * maxA;
    }

    if(configuredResponseCurveMode == "Linear (Default)") response = Response::Linear;
    if(configuredResponseCurveMode == "Relaxed to Aggressive") response = Response::RelaxedToAggressive;
    if(configuredResponseCurveMode == "Aggressive to Relaxed") response = Response::AggressiveToRelaxed;
    if(configuredResponseCurveMode == "Relaxed to Linear") response = Response::RelaxedToLinear;
    if(configuredResponseCurveMode == "Linear to Relaxed") response = Response::LinearToRelaxed;
    if(configuredResponseCurveMode == "Aggressive to Linear") response = Response::AggressiveToLinear;
    if(configuredResponseCurveMode == "Linear to Aggressive") response = Response::LinearToAggressive;

    switch(response) {

    case Response::Linear: {
      position = configuredProportionalSensitivity * ((position - configuredInnerDeadzone) * (saturationRadius) / (saturationRadius - configuredInnerDeadzone)) / position;
      break;
    }
    case Response::RelaxedToAggressive: case Response::AggressiveToRelaxed: {
      position = configuredProportionalSensitivity * pow(((position - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)), (a / b)) * saturationRadius * pow((sin(((position - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)) * Math::Pi / 2.0)), (2.0 * (b - a) / c)) / position;
      break;
    }
    case Response::RelaxedToLinear: case Response::AggressiveToLinear: {
      if(position <= switchDistance) {
        position = configuredProportionalSensitivity * pow(((position - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)), (a / b)) * saturationRadius * pow((sin(((position - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)) * Math::Pi / 2.0)), (2.0 * (b - a) / c)) / position;
      } else {
        position = configuredProportionalSensitivity * (position - configuredInnerDeadzone) * saturationRadius / (saturationRadius - configuredInnerDeadzone) / position;
      }
      break;
    }
    case Response::LinearToRelaxed: case Response::LinearToAggressive: {
      if(position <= switchDistance) {
        position = configuredProportionalSensitivity * (position - configuredInnerDeadzone) * saturationRadius / (saturationRadius - configuredInnerDeadzone) / position;
      } else {
        position = configuredProportionalSensitivity * pow(((position - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)), (a / b)) * saturationRadius * pow((sin(((position - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)) * Math::Pi / 2.0)), (2.0 * (b - a) / c)) / position;
      }
      break;
    }
    }
    return position;
  }
  
  auto processDeadzoneAndResponseCurve(double position, double length, double configuredInnerDeadzone, double saturationRadius, double offset, double configuredRangeNormalizedSwitchDistance, double configuredResponseStrength, double configuredProportionalSensitivity, string configuredResponseCurveMode) -> double {
    if(length <= configuredInnerDeadzone) {
      position = offset;
    } else {
      length = responseCurve(length, configuredInnerDeadzone, saturationRadius, offset, configuredRangeNormalizedSwitchDistance, configuredResponseStrength, configuredProportionalSensitivity, configuredResponseCurveMode);
      position = revisePosition(position, length, offset);
    }
    return position;
  }

  auto applyGateBoundaries(double cardinalMax, double diagonalMax, double positionX, double positionY, double offset, double& edgex, double& edgey) -> void {
    auto slope = (positionY - offset) / (positionX - offset);
    edgex = copysign(cardinalMax / (abs(slope) + (cardinalMax - diagonalMax) / diagonalMax), positionX - offset);
    edgey = copysign(min(abs(edgex * slope), cardinalMax / (1.0 / abs(slope) + (cardinalMax - diagonalMax) / diagonalMax)), positionY - offset);
    edgex = edgey / slope;
  }

  auto clampAxisToNearestBoundary(double position, double offset, double cardinalMax) -> double {
    if(abs(position - offset) > cardinalMax) position = copysign(cardinalMax, position - offset) + offset;
    return position;
  }

  auto counteractPrecisionError(double position) -> double {
    return copysign(abs(position) + 1e-09, position);
  }

protected:
  s64 _value = 0;
  s64 _minimum = -32768;
  s64 _maximum = +32767;

  string _outputStyle = "Octagon (Virtual) (Default)"; //for u8 analog stick, use "Square (Maximum Virtual)"
  f64 _customMaxOutput = 85.0; //for u8 analog stick, use 127.5
  f64 _deadzoneSize = 7.0; //for u8 analog stick, use 6.0
  f64 _maxOutputReducerOneFactor = 0.5;
  f64 _maxOutputReducerTwoFactor = 0.25;
  string _deadzoneShape = "Axial";
  f64 _proportionalSensitivity = 1.0;
  string _responseCurve = "Linear (Default)";
  f64 _rangeNormalizedSwitchDistance = 0.5;
  f64 _responseStrength = 0.0;
  bool _virtualNotch = false;
  f64 _notchLengthFromEdge = 0.1;
  f64 _notchAngularSnappingDistance = 0.0;

  recursive_mutex _mutex;
};
