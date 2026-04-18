#!/usr/bin/env perl
use strict; use warnings;

my @skip = (
    qr/^#include\s+"kgz_priv\.h"\s*$/,   # remove { #include "kgz_priv.h" }
    qr/^\s*#include\s*<stdint\.h>\s*$/,  # remove { #include <stdint.h> }
    qr/^\s*#include\s*<stddef\.h>\s*$/,  # remove { #include <stddef.h> }
    qr/^\s*#pragma\s+once\s*$/,          # remove { #pragma once }
    qr/^\s*\/\//,                        # remove single line comments { // }
    qr/\/\*.*\*\//,                      # remove single line comments { /* */ }
    qr/^\s*$/                            # remove empty lines
);

sub emit_file {
    my ($out, $path) = @_;
    open my $in, '<', $path or die "failed to open $path: $!";
    LINE: while (my $line = <$in>) {
        for my $re (@skip) {
            next LINE if $line =~ $re;
        }
        print $out $line;
    }
    close $in;
}

sub emit_string {
    my ($out, $text) = @_;
    print $out $text;
    print $out "\n";
}

sub step (&) { $_[0] }
my @steps = (
    # preamble
    step { my ($out) = @_; emit_string($out, "// KernelGZ -- Single Header Build -- Generated With comp2header.pl") },
    step { my ($out) = @_; emit_string($out, "// Read the included README.md for how to use this library") },
    step { my ($out) = @_; emit_string($out, "// Find the source code at https://git.sr.ht/~evalyn/kgz") },
    step { my ($out) = @_; emit_string($out, "/*") },
    step { my ($out) = @_; emit_file($out,   "LICENCE.txt") },
    step { my ($out) = @_; emit_string($out, "*/") },
    # includes
    step { my ($out) = @_; emit_string($out, "#pragma once") },
    step { my ($out) = @_; emit_string($out, "#include <stdint.h>") },
    step { my ($out) = @_; emit_string($out, "#include <stddef.h>") },
    step { my ($out) = @_; emit_string($out, "#define KGZ_SINGLE_HEADER") },
    # include the public user api
    step { my ($out) = @_; emit_file($out,   "./lib/kgz_pub.h") },
    # include the main source code
    step { my ($out) = @_; emit_string($out, "#ifdef KGZ_IMPLEMENTATION") },
    step { my ($out) = @_; emit_file($out,   "./lib/kgz_priv.h") },
    step { my ($out) = @_; emit_file($out,   "./lib/kgz_bitstream.c") },
    step { my ($out) = @_; emit_file($out,   "./lib/kgz_gzip.c") },
    step { my ($out) = @_; emit_file($out,   "./lib/kgz_deflate.c") },
    step { my ($out) = @_; emit_file($out,   "./lib/kgz_huffman.c") },
    step { my ($out) = @_; emit_file($out,   "./lib/kgz_buffer.c") },
    step { my ($out) = @_; emit_string($out, "#endif") },
);

open my $out, '>', 'kgz_singleheader.h' or die $!;
$_->($out) for @steps;
close $out or die "failed to save file";
