#!/usr/bin/perl

use warnings;
use strict;

use Getopt::Std;
use File::Spec::Functions;
use Time::ParseDate;
use POSIX;
use Data::Dumper;


# Default values
my %defaults = ( format  => '%Y/%m/%d [%H:%M:%S]',
		 basedir => catfile($ENV{HOME}, '.centerim'),
		);


# Helper functions
sub usage {
	# print error message if one is provided
	print STDERR $_[0] . "\n" if $_[0];

	print STDERR <<"EOT";
Usage: $0 [options]

Options:
\t-u <username>  User name to print history from. This must be an account
\t               name under the CenterIM basedir. This parameter is required.
\t                 * ICQ: 123456789
\t                 * MSN: mmsnuser
\t                 * Jab: jjabberuser
\t                 * Yah: yyahoouser
\t                 * ...
\t-s <start>     Timestamp to start from. This parameter is required.
\t               You can use timestamps or strings like '3 hours ago'.
\t-e <end>       Timestamp to end with. This parameter is required.
\t               Format: see -f.
\t-f <format>    Format to print timestamps in. Must be a strftime compatible
\t               string. This parameter is optional.
\t-b <basedir>   CenterIM basedir to use. This parameter is optional.
\t               Defaults to $defaults{basedir}.
\t-h             This help message.

Example:
\t$0 -u mfriendofmine -s "yesterday" -e "2 hours ago"
EOT
	exit;
}

my %options;

sub init {
	# get command line parameters and display usage if neccessary
	my %params;
	usage("Unrecognized parameter found.") unless getopt('u:s:e:f:b:h', \%params);
	usage() if $params{h};

	# put param$ters in %options hash and display usage if necessary
	# required parameters
	if ($params{u}) { $options{username} = $params{u} } else { usage("Required parameter -u not found.") };
	if ($params{s}) { $options{start}    = $params{s} } else { usage("Required parameter -s not found.") };
	if ($params{e}) { $options{end}      = $params{e} } else { usage("Required parameter -e not found.") };
	# optional parameters
	$options{format}  = $params{f} ? $params{f} : $defaults{format};
	$options{basedir} = $params{b} ? $params{b} : $defaults{basedir};

	$options{userdir} = catfile($options{basedir}, $options{username});

	# sanity checking of parameters
	$options{start} = parsedate($options{start}) || usage("Illegal date format for parameter -s.");
	$options{end}   = parsedate($options{end})   || usage("Illegal date format for parameter -e.");
	($options{start}, $options{end}) = ($options{end}, $options{start}) if $options{start} > $options{end};

	-d $options{basedir} || usage("Basedir '" . $options{basedir} . "' is not a directory.");
	-d $options{userdir} || usage("User dir '" . $options{userdir} . "' is not a directory.");
}

sub extract_history {
	my $histfile = catfile($options{userdir}, "history");

	my $histarray;
	open my $HISTORY, '<', $histfile;
	{
		local $/ = "\f\n";
		while (my $msg_struct = <$HISTORY>) {
			my ($msg_dir, $ts_sent, $ts_rcvd, $msg_text) =
				$msg_struct =~ m{ ^(IN|OUT)$ \n # direction of message
						  ^MSG$ \n      # message marker
						  ^(\d+)$ \n    # timestamp of sending
						  ^(\d+)$ \n    # timestamp of arriving
						  ^(.*)         # message content
						}xmso;

			next if !defined $msg_dir;	# no match, not a message
			next if $ts_sent < $options{start};
			last if $ts_sent > $options{end};

			# UNIXize newlines and remove end-of-record marker
			$msg_text =~ s/\r\n/\n/go;
			$msg_text =~ s/(?:\f|\n)+$//go;

			push @$histarray, { direction => $msg_dir, ts_sent => $ts_sent, ts_arrived => $ts_rcvd, message => $msg_text };
		}
	}

	return $histarray;
}

sub prettyprint_history {
	my ($history) = @_;

	for my $hist_item (@$history) {
		my $timestamp = strftime $options{format}, localtime $hist_item->{ts_sent};
		$timestamp .= " {rcv @ " . stftime($options{format}, localtime $hist_item->{ts_arrived}) . " }" if ($hist_item->{ts_sent} != $hist_item->{ts_arrived});

		my $direction = ($hist_item->{direction} eq 'IN') ? '<<<' : '>>>';
		my $message = $hist_item->{message};

		# properly indent multiple lines
		my $indent = ' ' x (length($timestamp) + 1 + length($direction) + 1);
		$message =~ s/\n/\n$indent/g;

		print "$timestamp $direction $message\n";
	}
}


# Main program
init();
my $history = extract_history();
prettyprint_history($history);
