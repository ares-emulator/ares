#include <myvision/myvision.hpp>

namespace ares::MyVision {

  PSG psg;
  #include "serialization.cpp"

  auto PSG::load(Node::Object parent)->void {
    node = parent->append < Node::Object > ("PSG");

    column = 0xff;

    stream = node->append < Node::Audio::Stream > ("PSG");
    stream->setChannels(1);
    stream->setFrequency(Constants::Colorburst::NTSC / 16);
  }

  auto PSG::unload()->void {
    node = {};
    stream = {};
    column = 0xff;
  }

  auto PSG::main()->void {
    auto channels = AY38910::clock();
    double output = 0.0;
    output += volume[channels[0]];
    output += volume[channels[1]];
    output += volume[channels[2]];
    stream->frame(output / 3.0);
    step(1);
  }

  auto PSG::step(uint clocks)->void {
    Thread::step(clocks);
    Thread::synchronize();
  }

  auto PSG::power()->void {
    AY38910::power();
    //real-hardware clock is unknown, we used the same value as spectrum and MSX as placeholder
    Thread::create(Constants::Colorburst::NTSC / 16, std::bind_front(&PSG::main, this));

    for (uint level: range(16)) {
      volume[level] = 1.0 / pow(2, 1.0 / 2 * (15 - level));
    }
  }

  auto PSG::readIO(n1 port)->n8 {
    if (port == 0) { // Controls
      n8 input = 0;
      if (!(column & 0x80)) { // ROW 0
        input.bit(3) = system.controls._13->value();
        input.bit(4) = system.controls.C->value();
        input.bit(5) = system.controls._9->value();
        input.bit(6) = system.controls._5->value();
        input.bit(7) = system.controls._1->value();
      }
      if (!(column & 0x40)) { // ROW 1
        input.bit(3) = system.controls.B->value();
        input.bit(5) = system.controls._12->value();
        input.bit(6) = system.controls._8->value();
        input.bit(7) = system.controls._4->value();
      }
      if (!(column & 0x20)) { // ROW 2
        input.bit(3) = system.controls._14->value();
        input.bit(4) = system.controls.D->value();
        input.bit(5) = system.controls._10->value();
        input.bit(6) = system.controls._6->value();
        input.bit(7) = system.controls._2->value();
      }
      if (!(column & 0x10)) { // ROW 3
        input.bit(3) = system.controls.A->value();
        input.bit(4) = system.controls.E->value();
        input.bit(5) = system.controls._11->value();
        input.bit(6) = system.controls._7->value();
        input.bit(7) = system.controls._3->value();
      }

      return input ^ 0xff;
    } else {
      return 0xff;
    }
  }

  auto PSG::writeIO(n1 port, n8 data)->void {
    if (port == 1) {
      column = data;
    }
  }

}
