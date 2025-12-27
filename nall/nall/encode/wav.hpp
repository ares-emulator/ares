#pragma once

namespace nall::Encode {

struct WAV {
private:
  static auto write_header(file_buffer& fp, const u32 channels, const u32 bits, const u32 frequency, const u32 samples) -> void {
    const u32 FMT_CHUNK_SIZE = 16;
    const u32 DATA_SIZE = samples * channels * bits / 8;

    // Master RIFF Chunk
    fp.write('R'); fp.write('I'); fp.write('F'); fp.write('F');
    fp.writel(4 + (8 + FMT_CHUNK_SIZE) + (8 + DATA_SIZE), 4);

    fp.write('W'); fp.write('A'); fp.write('V'); fp.write('E');

    fp.write('f'); fp.write('m'); fp.write('t'); fp.write(' ');
    fp.writel(FMT_CHUNK_SIZE, 4);
    fp.writel(1, 2);                                // Audio format (1 = PCM)
    fp.writel(channels, 2);                         // Number of channels
    fp.writel(frequency, 4);                        // Sample rate
    fp.writel(frequency * channels * bits / 8, 4);  // Byte rate
    fp.writel(channels * bits / 8, 2);              // Block align
    fp.writel(bits, 2);                             // Bits per sample

    fp.write('d'); fp.write('a'); fp.write('t'); fp.write('a');
    fp.writel(DATA_SIZE, 4);
  }

  template<typename SampleAccessor>
  static auto write_samples_interleaved(file_buffer &fp, u32 samples, SampleAccessor accessor) -> void {
    for(u32 i = 0; i < samples; ++i) {
      accessor(i);
    }
  }

public:
  template<typename SampleType>
  static auto stereo(const string& filename, std::span<const SampleType> left, std::span<const SampleType> right, u32 frequency) -> bool {
    if(left.size() != right.size()) return false;

    file_buffer fp;
    if(!fp.open(filename, file::mode::write)) return false;

    write_header(fp, 2, sizeof(SampleType) * 8, frequency, left.size());

    write_samples_interleaved(fp, left.size(), [&](u32 i) {
      fp.writel(left[i], sizeof(SampleType));
      fp.writel(right[i], sizeof(SampleType));
    });

    return true;
  }

  template<typename SampleType>
  static auto mono(const string& filename, std::span<const SampleType> samples, u32 frequency) -> bool {
    file_buffer fp;
    if(!fp.open(filename, file::mode::write)) return false;

    write_header(fp, 1, sizeof(SampleType) * 8, frequency, samples.size());
    write_samples_interleaved(fp, samples.size(), [&](u32 i) {
      fp.writel(samples[i], sizeof(SampleType));
    });

    return true;
  }
};

}
