use Math::Trig;
$SIZE = 64;

print "#define SINE_SIZE $SIZE\n\n";

print "static prog_uchar sine[SINE_SIZE/2] PROGMEM = {\n";
for $i (0..($SIZE/2-1)) {
    $v = int(128 + 127.5 * sin($i * 2 * pi / $SIZE));
    if ($v > 255) {
       $v = 255;
    }
    print $v;
    if ($i ne ($SIZE/2-1)) {
        print ",";
    }
}
print "\n};\n";