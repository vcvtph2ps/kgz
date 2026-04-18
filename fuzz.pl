#!/usr/bin/env perl
use strict; use warnings;
use Digest::SHA qw(sha256_hex);
use File::Copy qw(copy);

my $use_valgrind = 0;
my @args;
for my $arg (@ARGV) {
    if ($arg eq '--valgrind') {
        $use_valgrind = 1;
    } else {
        push @args, $arg;
    }
}

my $num_files = shift(@args) || 5;
my $min_size  = 1024;
my $max_size  = 5 * 1024**2;
my $src_bin = "./kgz_gunzip";
my $tmp_bin = "/tmp/kgz_gunzip";

print "running in /tmp\n";
copy($src_bin, $tmp_bin) or die "copy failed: $!";
chmod 0755, $tmp_bin or die "chmod failed: $!";
chdir("/tmp") or die "chdir failed: $!";

my @words = qw(
    the quick brown fox jumps over lazy dog and then ran back again
    hello world this is a test of gzip compression blocks working
    apple banana cherry date elderberry fig grape honeydew kiwi lemon
    one two three four five six seven eight nine ten eleven twelve
    red green blue yellow orange purple pink white black gray silver
    alpha beta gamma delta epsilon zeta eta theta iota kappa lambda
    north south east west left right up down inside outside around
    time space energy matter light dark fire water earth wind storm
    system kernel buffer memory page fault signal thread process fork
    function return value pointer struct array index loop break continue
    error warning debug info trace log level fatal critical notice
    network socket packet header payload checksum route bridge tunnel
    compress decompress inflate deflate block stream chunk offset size
);

my @sentence_templates = (
    "The %s and the %s were %s near the %s.",
    "When %s meets %s, the result is often %s and %s.",
    "Error: %s failed to %s after %s retries with code %s.",
    "Block %s: offset=%s length=%s checksum=%s",
    "Processing %s of %s: current=%s remaining=%s",
    "WARNING: %s exceeded threshold for %s in module %s (%s)",
    "LOG [%s] %s -> %s via %s",
    "packet seq=%s ack=%s window=%s flags=%s",
    "%s %s %s %s %s %s %s %s",
);

sub random_word {
    return $words[int(rand(scalar @words))];
}

sub random_sentence {
    my $tmpl = $sentence_templates[int(rand(scalar @sentence_templates))];
    $tmpl =~ s/%s/random_word()/ge;
    return $tmpl . "\n";
}

sub random_bytes {
    my ($size) = @_;
    return pack('C*', map { int(rand(256)) } 1 .. $size);
}

sub generate_text {
    my ($size) = @_;
    my $data = '';

    while (do { use bytes; length($data) } < $size) {
        my $style = int(rand(4));

        if ($style == 0) {
            my $phrase = random_sentence();
            my $repeat = int(rand(20)) + 5;
            $data .= $phrase x $repeat;

        } elsif ($style == 1) {
            my $count = int(rand(30)) + 10;
            for (1 .. $count) {
                my $ts  = sprintf("%04d-%02d-%02d %02d:%02d:%02d",
                    2020 + int(rand(5)), 1+int(rand(12)), 1+int(rand(28)),
                    int(rand(24)), int(rand(60)), int(rand(60)));
                my $lvl = (qw(INFO DEBUG WARN ERROR TRACE))[ int(rand(5)) ];
                $data .= "[$ts][$lvl] " . random_sentence();
            }

        } elsif ($style == 2) {
            my $count = int(rand(40)) + 10;
            for (1 .. $count) {
                $data .= random_sentence();
            }

        } else {
            my $blob_size = int(rand(4096)) + 64;
            $data .= random_bytes($blob_size);
        }
    }

    use bytes;
    return substr($data, 0, $size);
}

sub write_file {
    my ($filename, $size) = @_;
    open(my $fh, '>:raw', $filename) or die "cannot write $filename: $!";
    print $fh generate_text($size);
    close($fh);
}

sub sha256_file {
    my ($filename) = @_;
    open(my $fh, '<:raw', $filename) or die "cannot read $filename: $!";
    my $sha = Digest::SHA->new(256);
    $sha->addfile($fh);
    close($fh);
    return $sha->hexdigest;
}

for my $i (1 .. $num_files) {
    my $size = int(rand($max_size - $min_size)) + $min_size;
    my $file = "testfile_$i.bin";
    print "creating $file ($size bytes, plain-text)\n";
    write_file($file, $size);

    my $orig_hash = sha256_file($file);
    my $level = int(rand(9)) + 1;
    print "compressing $file with level $level\n";
    system("gzip -$level -c $file > $file.gz") == 0
        or die "gzip failed";

    print "running kgz_gunzip on $file.gz\n";
        my $cmd = $use_valgrind
            ? "valgrind --leak-check=full --error-exitcode=1 $tmp_bin $file.gz"
            : "$tmp_bin $file.gz";
        system($cmd) == 0
            or die $use_valgrind ? "kgz_gunzip failed or valgrind reported errors" : "kgz_gunzip failed";

    my $out_file = "$file.gz.ungzipped";
    unless (-e $out_file) {
        die "expected output $out_file not found";
    }

    my $new_hash = sha256_file($out_file);
    if ($orig_hash eq $new_hash) {
        print "OK: $file verified\n";
    } else {
        die "FAIL: $file mismatch\n";
    }
    print "-" x 40, "\n";
}
