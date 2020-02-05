#!/usr/bin/perl -w

#
#  ----------------------------------------------------
#  vsm - vector space model data similarity
#  ----------------------------------------------------
#
#  Copyright (c) 2008 Jason Bittel <jason.bittel@gmail.com>
#

# Take a query file and an input directory of text files. The text files are
# procesed and the top 10% are used to seed a markov chain generator. This is
# then used to create a new generation of files which are compared to the 
# original query file and the cycle starts anew. This might seem rather
# pointless, but it is good for testing and it is interesting to see how the
# generated files slowly converge towards the query file with different inputs.

# Portions of the Markov chain algorithm taken from _The Practice
# of Programming_ by Kernighan and Pike.

use strict;
use warnings;
use Getopt::Std;

# Defaults: can be overridden by command line switches
my $MAX_ITER = 10;   # Number of iterations to loop
my $MAX_FILES = 100; # Number of files to generate each loop
my $MAX_LEN = 500;   # Maximum length of generated files

my $VSM_BIN = "../vsm";

# Command line arguments
my %opts;
my $query_file;
my $input_dir;
my $max_iter;
my $max_files;
my $max_len;

# Global variables
my @dir;
my $file;
my $line;
my $exec;
my $iter = 1;
my %similarity;
my %chain;

&get_arguments();

# Bootstrap the similarity hash with text files in the input dir
opendir(DIR, $input_dir) or die "Error: Cannot open directory '$input_dir': $!\n";
@dir = map "$input_dir/$_", grep /\.txt$/, readdir(DIR);
closedir(DIR);

foreach $file (@dir) {
        $exec = `$VSM_BIN -t $query_file $file 2> /dev/null`;
        $exec =~ /^Similarity: (.*)$/;
        $similarity{$file} = $1 if $1;
}

# Seed, generate, and analyze ad nauseum
while ($iter <= $max_iter) {
        my $i = 1;
        my $j;
        my ($w1, $w2, $suf, $r, $t);
        
        print "$iter: Starting iteration\n";
        mkdir("./$iter", 0755);
        unless (-d "./$iter") { die "Error: Cannot create directory './$iter'\n"; }

        # Seed markov chain with results of previous run
        print "$iter: Seeding chain with data\n";
        %chain = ();
        $w1 = $w2 = "\n";
        foreach $file (sort { $similarity{$b} <=> $similarity{$a} } keys %similarity) {
                last if $i > ($max_files / 10); # Use top 10% of the files for seeding

                open(INFILE, "$file") or die "Error: Cannot open file '$file': $!\n";
                while ($line = <INFILE>) {
                        $line =~ s/[[:^print:]]//g; # Strip unprintable characters
                        foreach (split / /, $line) {
                                next if $_ eq "";
                                push (@{$chain{$w1}{$w2}}, $_);
                                ($w1, $w2) = ($w2, $_);
                        }
                }
                close(INFILE);

                $i++;
        }
        push (@{$chain{$w1}{$w2}}, "\n");

        # Spew files into iter directory
        print "$iter: Creating output files\n";
        for ($i = 1; $i <= $max_files; $i++) {
                $w1 = $w2 = "\n";

                open(DATAFILE, ">./$iter/$i.txt") or die "Error: Cannot open file './$iter/$i.txt': $!\n";
                for ($j = 0; $j < $max_len; $j++) {
                        $suf = $chain{$w1}{$w2};
                        $r = int(rand @$suf);
                        last if (($t = $suf->[$r]) eq "\n");
                        print DATAFILE "$t ";
                        ($w1, $w2) = ($w2, $t);
                        if (($j % 20) == 0) { print DATAFILE "\n"; } # Create lines 20 words long
                }
                close(DATAFILE);
        }

        # Process all files in iter dir
        print "$iter: Processing all generated files\n";
        %similarity = ();
        for ($i = 1; $i <= $max_files; $i++) {
                $exec = `$VSM_BIN -t $query_file ./$iter/$i.txt 2> ./$iter/$i.log`;
                $exec =~ /^Similarity: (.*)$/;
                $similarity{"./$iter/$i.txt"} = $1 if $1;
        }

        # Save contents of similarity hash to log file in iter dir
        print "$iter: Saving iteration log file\n";
        open(LOGFILE, ">./$iter/iter$iter.log") or die "Error: Cannot open file './$iter/iter$iter.log': $!\n";
        foreach $file (sort { $similarity{$b} <=> $similarity{$a} } keys %similarity) {
                print LOGFILE "$file\t$similarity{$file}\n";
        }
        close(LOGFILE);

        $iter++;
}

# -----------------------------------------------------------------------------
# Retrieve and process command line arguments
# -----------------------------------------------------------------------------
sub get_arguments {
        getopts('hd:f:i:l:t:', \%opts) or &print_usage();

        # Print help/usage information to the screen if necessary
        &print_usage() if ($opts{h});

        # Copy command line arguments to internal variables
        &print_usage() if (!$opts{d} or !$opts{q});
        $input_dir = $opts{d};
        $query_file = $opts{t};

        $max_files = $MAX_FILES unless ($max_files = $opts{f});
        $max_iter = $MAX_ITER unless ($max_iter = $opts{i});
        $max_len = $MAX_LEN unless ($max_len = $opts{l});

        return;
}

# -----------------------------------------------------------------------------
# Print usage/help information to the screen and exit
# -----------------------------------------------------------------------------
sub print_usage {
        die <<USAGE;
Usage: $0 [-h] [-fil] -d dir -t file

REQUIRED
  -d   directory with seed files
  -t   file containing query terms

OPTIONAL
  -f   number of files to generate each iteration
  -h   print this help information and exit
  -i   number of iterations
  -l   maximum length of generated files

USAGE
}
