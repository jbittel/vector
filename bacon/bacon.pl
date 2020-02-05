#!/usr/bin/perl -w

#
#  ----------------------------------------------------
#  vsm - vector space model data similarity
#  ----------------------------------------------------
#
#  Copyright (c) 2008 Jason Bittel <jason.bittel@gmail.com>
#

use strict;
use warnings;
use Getopt::Std;

# Command line arguments
my %opts;

my %active_conn = ();
my $tcpick_args = "-r /home/jbittel/tmp/http_port_80 -v0 -bU";
my $tcpick_filter = "tcp port 80";
my ($conn_num, $status, $src_ip, $dst_ip);
my $uri;

open(TCPICK, "tcpick $tcpick_args '$tcpick_filter' |") or die "Error: Cannot open a pipe to tcpick\n";

while (<TCPICK>) {
#        print $_;

        if (/^(\d+)\s+([A-Z-]+)\s+([\d\.]+):.* > ([\d\.]+)/) {
                $conn_num = $1;
                $status = $2;
                $src_ip = $3;
                $dst_ip = $4;

                if ($status eq "ESTABLISHED") {
                        &new_connection($conn_num, $src_ip, $dst_ip);
                } elsif (($status eq "RESET") or ($status eq "CLOSED")) {
                        &end_connection($conn_num);
                }
        } elsif (/^GET\s+([^\s]+)\s+HTTP\/1\.\d\r$/) {
                $uri = $1;
                print "URI requested: $uri\n";
        } elsif (/^HTTP\/1\.\d\s+\d+\s+[A-Z]+\r$/) {
                print "Got server response\n";
        }
}

close(TCPICK);

# -----------------------------------------------------------------------------
#
# -----------------------------------------------------------------------------
sub new_connection {
        my $conn_num = shift;
        my $src_ip = shift;
        my $dst_ip = shift;

        if (exists $active_conn{$conn_num}) {
                print "Error: Connection $conn_num already exists\n";
                return;
        }
        
        $active_conn{$conn_num}->{"src_ip"} = $src_ip;
        $active_conn{$conn_num}->{"dst_ip"} = $dst_ip;

        print "Created connection $conn_num\n";

        return;
}

# -----------------------------------------------------------------------------
#
# -----------------------------------------------------------------------------
sub end_connection {
        my $conn_num = shift;

        if (!exists $active_conn{$conn_num}) {
                print "Error: Connection $conn_num doesn't exist\n";
                return;
        }

        delete $active_conn{$conn_num};

        print "Terminated connection $conn_num\n";

        return;
}

# -----------------------------------------------------------------------------
# Retrieve and process command line arguments
# -----------------------------------------------------------------------------
sub get_arguments {
        getopts('h', \%opts) or &print_usage();

        # Print help/usage information to the screen if necessary
        &print_usage() if ($opts{h});

        return;
}

# -----------------------------------------------------------------------------
# Print usage/help information to the screen and exit
# -----------------------------------------------------------------------------
sub print_usage {
        die <<USAGE;
Usage: $0 [-h]

OPTIONAL
  -h ... print this help information and exit
USAGE
}
