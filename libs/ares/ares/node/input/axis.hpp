struct Axis : Input {
  DeclareClass(Axis, "input.axis")
  using Input::Input;

  auto value() const -> s64 { return _value; }
  auto setValue(s64 value) -> void { _value = value; }

    auto setOperatingRange(s16 position, double saturationRadius, double offset) -> double {
    return position * saturationRadius / 32767.0 + offset;
  }
  
  auto processDeadzoneAndResponseCurve(double position, double innerDeadzone, double cardinalMax, double offset) -> double {
    auto lengthAbsolute = abs(position - offset);
    if(lengthAbsolute <= innerDeadzone) {
      position = offset;
    } else {
      lengthAbsolute = (lengthAbsolute - innerDeadzone) * (cardinalMax) / (cardinalMax - innerDeadzone) / lengthAbsolute;
      position = (position - offset) * lengthAbsolute + offset;
    }
    return position;
  }

  auto revisePosition(double position, double length, double saturationRadius, double offset) -> double {
    if(length > saturationRadius) {
      length = saturationRadius / length;
      position = (position - offset) * length + offset;
    }
    return position;
  }

  auto applyGateBoundaries(double innerDeadzone, double cardinalMax, double diagonalMax, double positionX, double positionY, double offset, double& boundedPositionX, double& boundedPositionY) -> void {
    if(positionX != offset && positionY != offset) {
      auto slope = (positionY - offset) / (positionX - offset);
      auto edgex = copysign(cardinalMax / (abs(slope) + (cardinalMax - diagonalMax) / diagonalMax), positionX);
      auto edgey = copysign(min(abs(edgex * slope), cardinalMax / (1.0 / abs(slope) + (cardinalMax - diagonalMax) / diagonalMax)), positionY);
      edgex = edgey / slope;

      auto distanceToEdge = hypot(edgex, edgey);

      auto length = hypot(positionX - offset, positionY - offset);
      if(length > distanceToEdge) {
        positionX = edgex + offset;
        positionY = edgey + offset;
      }
    }
    boundedPositionX = positionX;
    boundedPositionY = positionY;
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
};
