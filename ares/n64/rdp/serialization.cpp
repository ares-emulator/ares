auto RDP::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(command.start);
  s(command.end);
  s(command.current);
  s(command.clock);
  s(command.bufferBusy);
  s(command.pipeBusy);
  s(command.tmemBusy);
  s(command.source);
  s(command.freeze);
  s(command.flush);
  s(command.startValid);
  s(command.endValid);
  s(command.startGclk);
  s(command.ready);
  s(command.crashed);

  s(edge.lmajor);
  s(edge.level);
  s(edge.tile);
  s(edge.y.hi);
  s(edge.y.md);
  s(edge.y.lo);
  s(edge.x.hi.c.i);
  s(edge.x.hi.c.f);
  s(edge.x.hi.s.i);
  s(edge.x.hi.s.f);
  s(edge.x.md.c.i);
  s(edge.x.md.c.f);
  s(edge.x.md.s.i);
  s(edge.x.md.s.f);
  s(edge.x.lo.c.i);
  s(edge.x.lo.c.f);
  s(edge.x.lo.s.i);
  s(edge.x.lo.s.f);

  s(shade.r.c.i);
  s(shade.r.c.f);
  s(shade.r.x.i);
  s(shade.r.x.f);
  s(shade.r.y.i);
  s(shade.r.y.f);
  s(shade.r.e.i);
  s(shade.r.e.f);
  s(shade.g.c.i);
  s(shade.g.c.f);
  s(shade.g.x.i);
  s(shade.g.x.f);
  s(shade.g.y.i);
  s(shade.g.y.f);
  s(shade.g.e.i);
  s(shade.g.e.f);
  s(shade.b.c.i);
  s(shade.b.c.f);
  s(shade.b.x.i);
  s(shade.b.x.f);
  s(shade.b.y.i);
  s(shade.b.y.f);
  s(shade.b.e.i);
  s(shade.b.e.f);
  s(shade.a.c.i);
  s(shade.a.c.f);
  s(shade.a.x.i);
  s(shade.a.x.f);
  s(shade.a.y.i);
  s(shade.a.y.f);
  s(shade.a.e.i);
  s(shade.a.e.f);

  s(texture.s.c.i);
  s(texture.s.c.f);
  s(texture.s.x.i);
  s(texture.s.x.f);
  s(texture.s.y.i);
  s(texture.s.y.f);
  s(texture.s.e.i);
  s(texture.s.e.f);
  s(texture.t.c.i);
  s(texture.t.c.f);
  s(texture.t.x.i);
  s(texture.t.x.f);
  s(texture.t.y.i);
  s(texture.t.y.f);
  s(texture.t.e.i);
  s(texture.t.e.f);
  s(texture.w.c.i);
  s(texture.w.c.f);
  s(texture.w.x.i);
  s(texture.w.x.f);
  s(texture.w.y.i);
  s(texture.w.y.f);
  s(texture.w.e.i);
  s(texture.w.e.f);

  s(zbuffer.d.i);
  s(zbuffer.d.f);
  s(zbuffer.x.i);
  s(zbuffer.x.f);
  s(zbuffer.y.i);
  s(zbuffer.y.f);
  s(zbuffer.e.i);
  s(zbuffer.e.f);

  s(rectangle.tile);
  s(rectangle.x.hi);
  s(rectangle.x.lo);
  s(rectangle.y.hi);
  s(rectangle.y.lo);
  s(rectangle.s.i);
  s(rectangle.s.f);
  s(rectangle.t.i);
  s(rectangle.t.f);

  s(other.atomicPrimitive);
  s(other.reserved1);
  s(other.cycleType);
  s(other.perspective);
  s(other.detailTexture);
  s(other.sharpenTexture);
  s(other.lodTexture);
  s(other.tlut);
  s(other.tlutType);
  s(other.sampleType);
  s(other.midTexel);
  s(other.bilerp);
  s(other.convertOne);
  s(other.colorKey);
  s(other.colorDitherMode);
  s(other.alphaDitherMode);
  s(other.reserved2);
  s(other.blend1a);
  s(other.blend1b);
  s(other.blend2a);
  s(other.blend2b);
  s(other.reserved3);
  s(other.forceBlend);
  s(other.alphaCoverage);
  s(other.coverageXalpha);
  s(other.zMode);
  s(other.coverageMode);
  s(other.colorOnCoverage);
  s(other.imageRead);
  s(other.zUpdate);
  s(other.zCompare);
  s(other.antialias);
  s(other.zSource);
  s(other.ditherAlpha);
  s(other.alphaCompare);

  s(fog.red);
  s(fog.green);
  s(fog.blue);
  s(fog.alpha);

  s(blend.red);
  s(blend.green);
  s(blend.blue);
  s(blend.alpha);

  s(primitive.minimum);
  s(primitive.fraction);
  s(primitive.red);
  s(primitive.green);
  s(primitive.blue);
  s(primitive.alpha);

  s(environment.red);
  s(environment.green);
  s(environment.blue);
  s(environment.alpha);

  s(combine.mul.color);
  s(combine.mul.alpha);
  s(combine.add.color);
  s(combine.add.alpha);
  s(combine.sba.color);
  s(combine.sba.alpha);
  s(combine.sbb.color);
  s(combine.sbb.alpha);

  s(tlut.index);
  s(tlut.s.lo);
  s(tlut.s.hi);
  s(tlut.t.lo);
  s(tlut.t.hi);

  s(load_.block.index);
  s(load_.block.s.lo);
  s(load_.block.s.hi);
  s(load_.block.t.lo);
  s(load_.block.t.hi);
  s(load_.tile.index);
  s(load_.tile.s.lo);
  s(load_.tile.s.hi);
  s(load_.tile.t.lo);
  s(load_.tile.t.hi);

  s(tileSize.index);
  s(tileSize.s.lo);
  s(tileSize.s.hi);
  s(tileSize.t.lo);
  s(tileSize.t.hi);

  s(tile.format);
  s(tile.size);
  s(tile.line);
  s(tile.address);
  s(tile.index);
  s(tile.palette);
  s(tile.s.clamp);
  s(tile.s.mirror);
  s(tile.s.mask);
  s(tile.s.shift);
  s(tile.t.clamp);
  s(tile.t.mirror);
  s(tile.t.mask);
  s(tile.t.shift);

  s(set.fill.color);
  s(set.texture.format);
  s(set.texture.size);
  s(set.texture.width);
  s(set.texture.dramAddress);
  s(set.mask.dramAddress);
  s(set.color.format);
  s(set.color.size);
  s(set.color.width);
  s(set.color.dramAddress);

  s(primitiveDepth.z);
  s(primitiveDepth.deltaZ);

  s(scissor.field);
  s(scissor.odd);
  s(scissor.x.lo);
  s(scissor.x.hi);
  s(scissor.y.lo);
  s(scissor.y.hi);

  for(auto& k : convert.k) s(k);

  s(key.r.width);
  s(key.r.center);
  s(key.r.scale);
  s(key.g.width);
  s(key.g.center);
  s(key.g.scale);
  s(key.b.width);
  s(key.b.center);
  s(key.b.scale);

  s(fillRectangle_.x.lo);
  s(fillRectangle_.x.hi);
  s(fillRectangle_.y.lo);
  s(fillRectangle_.y.hi);

  s(io.bist.check);
  s(io.bist.go);
  s(io.bist.done);
  s(io.bist.fail);

  s(io.test.enable);
  s(io.test.address);
  for(auto& d : io.test.data) s(d);
}
