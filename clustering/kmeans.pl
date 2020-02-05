#!/usr/bin/perl -w

#
#  ----------------------------------------------------
#  vsm - vector space model data similarity
#  ----------------------------------------------------
#
#  Copyright (c) 2008 Jason Bittel <jason.bittel@gmail.com>
#

#
# Take a file containing similarity data and cluster the
# points into two groups using the k-means clustering
# algorithm. The initial placement for the cluster points
# is at the min/max points so as to place the points into
# high/low clusters. All points less than or equal to zero
# are discarded.
#
# K-means algorithm code originally taken from:
#     http://www.perlmonks.org/?node_id=541000
# Many subsequent modifications and changes have been made
# from the original version.
#

use strict;
use warnings;
use Getopt::Std;

# Defaults: can be overridden by command line switches
my $TOLERANCE = 0.001;

# Command line arguments
my %opts;
my $data_file;
my $tolerance;

my @data = ();
my @center = ();
my @cluster = ();
my $similarity;
my $max_similarity = 0;
my $min_similarity = 99999;
my $diff;

my $num_lines = 0;
my $sum = 0;

&get_arguments();

# Read data from file
open(INFILE, "$data_file") or die "Error: Cannot open file '$data_file': $!\n";
        foreach my $line (<INFILE>) {
                chomp $line;
                # TODO: we need a more generic method for parsing files
                $similarity = (split(/ /, $line))[3];
                next if $similarity <= 0;

                $sum += $similarity;
                $num_lines++;

                # Determine min/max values from input data
                if ($similarity < $min_similarity) {
                        $min_similarity = $similarity;
                }
                if ($similarity > $max_similarity) {
                        $max_similarity = $similarity;
                }

                push(@data, $similarity);
        }
close(INFILE);

my @temp = sort @data;
@data = @temp;

# We use only two centers, starting with the highest and lowest
# similarity scores; this ought to divide the data into two
# distinct clusters that separate high and low similarity scores
push(@center, $min_similarity);
push(@center, $max_similarity);

do {
        $diff = 0;
        @cluster = ();

        # Assign points to nearest center
        foreach my $point (@data) {
                my $closest = 0;
                my $dist = abs $point - $center[$closest];
                foreach my $idx (1..$#center) {
                        if (abs $point - $center[$idx] < $dist) {
                                $dist = abs $point - $center[$idx];
                                $closest = $idx;
                        }
                }
                push @cluster, [$point, $closest];
        }

        # Compute new centers
        foreach my $center_idx (0..$#center) {
                my @members = grep { $_->[1] == $center_idx } @cluster;
                my $sum = 0;
                foreach my $member (@members) {
                        $sum += $member->[0];
                }
                my $new_center = @members ? $sum / @members : $center[$center_idx];
                $diff += abs $center[$center_idx] - $new_center;
                $center[$center_idx] = $new_center;
        }
} while ($diff > $tolerance);

# Dump everything to the screen
print "Arithmetic mean: ".sprintf("%.4f", $sum / $num_lines)."\n";

print "\nClusters are:\n";
foreach my $point (0..$#cluster) {
        print "$cluster[$point][0]\t$cluster[$point][1]\n";
}

print "\nCenters are:\n";
foreach my $center_idx (0..$#center) {
        print sprintf("%.4f", $center[$center_idx])."\t$center_idx\n";
}

# -----------------------------------------------------------------------------
# Retrieve and process command line arguments
# -----------------------------------------------------------------------------
sub get_arguments {
        getopts('hf:t:', \%opts) or &print_usage();

        # Print help/usage information to the screen if necessary
        &print_usage() if ($opts{h});
        &print_usage() unless ($opts{f});

        # Copy command line arguments to internal variables
        $data_file = $opts{f};

        $tolerance = $TOLERANCE unless ($tolerance = $opts{t});

        return;
}

# -----------------------------------------------------------------------------
# Print usage/help information to the screen and exit
# -----------------------------------------------------------------------------
sub print_usage {
        die <<USAGE;
Usage: $0 [-h] [-t] -f file

REQUIRED
  -f   input fetched db file

OPTIONAL
  -h   print this help information and exit
  -t   tolerance for ending the loop

USAGE
}
