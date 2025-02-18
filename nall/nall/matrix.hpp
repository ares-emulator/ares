#pragma once

namespace nall {

template<typename T, u32 Rows, u32 Cols>
struct Matrix {
  static_assert(Rows > 0 && Cols > 0);

  Matrix() = default;
  Matrix(const Matrix&) = default;
  Matrix(const initializer_list<T>& source) {
    u32 index = 0;
    for(auto& value : source) {
      if(index >= Rows * Cols) break;
      values[index / Cols][index % Cols] = value;
    }
  }

  operator array_span<T>() { return {values, Rows * Cols}; }
  operator array_view<T>() const { return {values, Rows * Cols}; }

  //1D matrices (for polynomials, etc)
  auto operator[](u32 row) -> T& { return values[row][0]; }
  auto operator[](u32 row) const -> T { return values[row][0]; }

  //2D matrices
  auto operator()(u32 row, u32 col) -> T& { return values[row][col]; }
  auto operator()(u32 row, u32 col) const -> T { return values[row][col]; }

  //operators
  auto operator+() const -> Matrix {
    Matrix result;
    for(u32 row : range(Rows)) {
      for(u32 col : range(Cols)) {
        result(row, col) = +target(row, col);
      }
    }
    return result;
  }

  auto operator-() const -> Matrix {
    Matrix result;
    for(u32 row : range(Rows)) {
      for(u32 col : range(Cols)) {
        result(row, col) = -target(row, col);
      }
    }
    return result;
  }

  auto operator+(const Matrix& source) const -> Matrix {
    Matrix result;
    for(u32 row : range(Rows)) {
      for(u32 col : range(Cols)) {
        result(row, col) = target(row, col) + source(row, col);
      }
    }
    return result;
  }

  auto operator-(const Matrix& source) const -> Matrix {
    Matrix result;
    for(u32 row : range(Rows)) {
      for(u32 col : range(Cols)) {
        result(row, col) = target(row, col) - source(row, col);
      }
    }
    return result;
  }

  auto operator*(T source) const -> Matrix {
    Matrix result;
    for(u32 row : range(Rows)) {
      for(u32 col : range(Cols)) {
        result(row, col) = target(row, col) * source;
      }
    }
    return result;
  }

  auto operator/(T source) const -> Matrix {
    Matrix result;
    for(u32 row : range(Rows)) {
      for(u32 col : range(Cols)) {
        result(row, col) = target(row, col) / source;
      }
    }
    return result;
  }

  //warning: matrix multiplication is not commutative!
  template<u32 SourceRows, u32 SourceCols>
  auto operator*(const Matrix<T, SourceRows, SourceCols>& source) const -> Matrix<T, Rows, SourceCols> {
    static_assert(Cols == SourceRows);
    Matrix<T, Rows, SourceCols> result;
    for(u32 y : range(Rows)) {
      for(u32 x : range(SourceCols)) {
        T sum{};
        for(u32 z : range(Cols)) {
          sum += target(y, z) * source(z, x);
        }
        result(y, x) = sum;
      }
    }
    return result;
  }

  template<u32 SourceRows, u32 SourceCols>
  auto operator/(const Matrix<T, SourceRows, SourceCols>& source) const -> maybe<Matrix<T, Rows, SourceCols>> {
    static_assert(Cols == SourceRows && SourceRows == SourceCols);
    if(auto inverted = source.invert()) return operator*(inverted());
    return {};
  }

  auto& operator+=(const Matrix& source) { return *this = operator+(source); }
  auto& operator-=(const Matrix& source) { return *this = operator-(source); }
  auto& operator*=(T source) { return *this = operator*(source); }
  auto& operator/=(T source) { return *this = operator/(source); }
  template<u32 SourceRows, u32 SourceCols>
  auto& operator*=(const Matrix<T, SourceRows, SourceCols>& source) { return *this = operator*(source); }
  //matrix division is not always possible (when matrix cannot be inverted), so operator/= is not provided

  //algorithm: Gauss-Jordan
  auto invert() const -> maybe<Matrix> {
    static_assert(Rows == Cols);
    Matrix source = *this;
    Matrix result = identity();

    const auto add = [&](u32 targetRow, u32 sourceRow, T factor = 1) {
      for(u32 col : range(Cols)) {
        result(targetRow, col) += result(sourceRow, col) * factor;
        source(targetRow, col) += source(sourceRow, col) * factor;
      }
    };

    const auto sub = [&](u32 targetRow, u32 sourceRow, T factor = 1) {
      for(u32 col : range(Cols)) {
        result(targetRow, col) -= result(sourceRow, col) * factor;
        source(targetRow, col) -= source(sourceRow, col) * factor;
      }
    };

    const auto mul = [&](u32 row, T factor) {
      for(u32 col : range(Cols)) {
        result(row, col) *= factor;
        source(row, col) *= factor;
      }
    };

    for(u32 i : range(Cols)) {
      if(source(i, i) == 0) {
        for(u32 row : range(Rows)) {
          if(source(row, i) != 0) {
            add(i, row);
            break;
          }
        }
        //matrix is not invertible:
        if(source(i, i) == 0) return {};
      }

      mul(i, T{1} / source(i, i));
      for(u32 row : range(Rows)) {
        if(row == i) continue;
        sub(row, i, source(row, i));
      }
    }

    return result;
  }

  auto transpose() const -> Matrix<T, Cols, Rows> {
    Matrix<T, Cols, Rows> result;
    for(u32 row : range(Rows)) {
      for(u32 col : range(Cols)) {
        result(col, row) = target(row, col);
      }
    }
    return result;
  }

  static auto identity() -> Matrix {
    static_assert(Rows == Cols);
    Matrix result;
    for(u32 row : range(Rows)) {
      for(u32 col : range(Cols)) {
        result(row, col) = row == col;
      }
    }
    return result;
  }

  //debugging function: do not use in production code
  template<u32 Pad = 0>
  auto _print() const -> void {
    for(u32 row : range(Rows)) {
      nall::print("[ ");
      for(u32 col : range(Cols)) {
        nall::print(pad(target(row, col), Pad, ' '), " ");
      }
      nall::print("]\n");
    }
  }

protected:
  //same as operator(), but with easier to read syntax inside Matrix class
  auto target(u32 row, u32 col) -> T& { return values[row][col]; }
  auto target(u32 row, u32 col) const -> T { return values[row][col]; }

  T values[Rows][Cols]{};
};

}
