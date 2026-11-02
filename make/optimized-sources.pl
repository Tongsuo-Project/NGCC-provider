# Loaded by ./config only when author optimized implementations are selected.
package NGCC::OptimizedSources;
use strict;
use warnings;
use Cwd qw(abs_path);
use Digest::SHA;
use Encode qw(decode FB_CROAK);
use File::Basename qw(dirname);
use File::Copy qw(copy);
use File::Path qw(make_path);
use File::Spec;
use File::Temp;
use IO::Uncompress::Unzip;
use JSON::PP qw(decode_json);

sub read_bytes {
    my ($path) = @_;
    open my $file, '<:raw', $path or die "$path: $!\n";
    local $/;
    return <$file>;
}

sub file_sha256 {
    my ($path) = @_;
    open my $file, '<:raw', $path or die "$path: $!\n";
    return Digest::SHA->new(256)->addfile($file)->hexdigest;
}

sub destination_path {
    my ($root, $relative) = @_;
    my @parts = split m{/}, $relative, -1;
    die "Invalid author source destination: $relative\n"
        if File::Spec->file_name_is_absolute($relative)
            || $relative =~ /[\\\r\n\0]/
            || grep { $_ eq '' || $_ eq '.' || $_ eq '..' } @parts;
    my $path = $root;
    for my $part (@parts) {
        $path = File::Spec->catfile($path, $part);
        die "Refusing a symlink in author source destination: $relative\n" if -l $path;
    }
    return $path;
}

sub prepare {
    my ($archive_root) = @_;
    my $destination = abs_path('ngcc/candidates');
    die "Initialize ngcc/candidates with git submodule update --init first\n"
        unless defined $destination && -d "$destination/provenance";
    $archive_root = File::Spec->rel2abs($archive_root);
    my $origins = decode_json(read_bytes('make/optimized-origins.json'))->{archives};
    my (%expected, %seen, %missing, %needed);
    for my $line (split /\n/, read_bytes('make/optimized-sources.sha256')) {
        my ($sha, $relative) = $line =~ /^([0-9a-f]{64})  (.+)$/;
        die "Invalid optimized source checksum line\n" unless defined $relative;
        die "Duplicate optimized source: $relative\n" if exists $expected{$relative};
        $expected{$relative} = $sha;
        my $path = destination_path($destination, $relative);
        if (-e $path) {
            die "Existing author source differs; refusing to overwrite: $relative\n"
                unless -f $path && file_sha256($path) eq $sha;
        } else {
            $missing{$relative} = 1;
        }
    }
    for my $archive_sha (sort keys %$origins) {
        die "Invalid optimized archive checksum\n" unless $archive_sha =~ /^[0-9a-f]{64}$/;
        for my $pair (@{$origins->{$archive_sha}{files}}) {
            my ($relative, $member) = @$pair;
            die "Origin catalog does not match checksum manifest: $relative\n"
                unless exists $expected{$relative} && !$seen{$relative}++;
            push @{$needed{$archive_sha}{$member}}, $relative if $missing{$relative};
        }
    }
    die "Origin catalog and checksum manifest have different file counts\n"
        unless keys(%seen) == keys(%expected);

    # Stage every missing member before changing the source tree or configuration.
    # Installed, verified files do not require the original archives to be present.
    if (%missing) {
        my $ignore = destination_path($destination, '.gitignore');
        my $marker = "\n# Author optimized implementation imports\n";
        my ($original_ignore) = split /\Q$marker\E/, read_bytes($ignore), 2;
        $original_ignore //= '';
        for my $archive_sha (sort keys %needed) {
            my $archive = File::Spec->catfile($archive_root, $origins->{$archive_sha}{path});
            die "Missing optimized source archive: $archive\n"
              . "Use ./config --implementation=optimized --with-optimized-archives=DIR\n"
                unless -f $archive;
            die "Archive checksum mismatch: $archive\n" unless file_sha256($archive) eq $archive_sha;
        }
        {
            local $| = 1;
            printf "Importing %d author source files from %d verified archives...\n",
                scalar(keys %missing), scalar(keys %needed);
        }
        my $staging = File::Temp->newdir('ngcc-optimized-XXXXXX', TMPDIR => 1);
        for my $archive_sha (sort keys %needed) {
            my $archive = File::Spec->catfile($archive_root, $origins->{$archive_sha}{path});
            my $wanted = $needed{$archive_sha};
            my %found;
            my $zip = IO::Uncompress::Unzip->new($archive, Efs => 1, Strict => 1)
                or die "Cannot read ZIP $archive: $IO::Uncompress::Unzip::UnzipError\n";
            my $status = 1;
            while ($status > 0) {
                my $header = $zip->getHeaderInfo();
                my $member = $header->{efs} ? $header->{Name}
                           : decode('cp437', $header->{Name}, FB_CROAK);
                if (exists $wanted->{$member}) {
                    die "Duplicate source member in ZIP: $member\n" if $found{$member}++;
                    my @relatives = @{$wanted->{$member}};
                    my $relative = $relatives[0];
                    my $output = File::Spec->catfile("$staging", $relative);
                    make_path(dirname($output));
                    open my $file, '>:raw', $output or die "$output: $!\n";
                    my $digest = Digest::SHA->new(256);
                    while (1) {
                        my $length = $zip->read(my $buffer, 65536);
                        die "Cannot extract $member: $IO::Uncompress::Unzip::UnzipError\n"
                            if $length < 0;
                        last if $length == 0;
                        $digest->add($buffer);
                        print {$file} $buffer or die "$output: $!\n";
                    }
                    close $file or die "$output: $!\n";
                    my $sha = $digest->hexdigest;
                    for my $path (@relatives) {
                        die "Author member checksum mismatch: $member\n" unless $sha eq $expected{$path};
                        next if $path eq $relative;
                        my $alias = File::Spec->catfile("$staging", $path);
                        make_path(dirname($alias));
                        copy($output, $alias) or die "$alias: $!\n";
                    }
                }
                $status = $zip->nextStream();
            }
            die "Cannot read ZIP $archive: $IO::Uncompress::Unzip::UnzipError\n" if $status < 0;
            die "Missing source member in ZIP: $_\n" for grep { !$found{$_} } keys %$wanted;
            $zip->close();
        }
        for my $relative (sort keys %missing) {
            my $output = destination_path($destination, $relative);
            # Refuse overwrites even if a file appeared while archives were read.
            die "Author source appeared during import: $relative\n" if -e $output;
            make_path(dirname($output));
            # The staging directory may be on a different filesystem.
            my ($file, $temporary) = File::Temp::tempfile('.ngcc-import-XXXXXX', DIR => dirname($output), UNLINK => 1);
            binmode $file;
            open my $input, '<:raw', File::Spec->catfile("$staging", $relative) or die "$relative: $!\n";
            while (1) {
                my $length = read($input, my $buffer, 65536);
                die "$relative: $!\n" unless defined $length;
                last unless $length;
                print {$file} $buffer or die "$temporary: $!\n";
            }
            close $input;
            close $file or die "$temporary: $!\n";
            chmod 0644, $temporary or die "$temporary: $!\n";
            rename $temporary, $output or die "$output: $!\n";
        }
        my ($file, $temporary) = File::Temp::tempfile('.ngcc-ignore-XXXXXX', DIR => $destination, UNLINK => 1);
        binmode $file;
        print {$file} $original_ignore, $marker, map { "!/$_\n" } sort keys %expected
            or die "$temporary: $!\n";
        close $file or die "$temporary: $!\n";
        chmod 0644, $temporary or die "$temporary: $!\n";
        destination_path($destination, '.gitignore');
        rename $temporary, $ignore or die "$ignore: $!\n";
    }
    printf "Verified %d author optimized source files; imported %d from %d pinned archives\n",
        scalar(keys %expected), scalar(keys %missing), scalar(keys %needed);
}

1;
