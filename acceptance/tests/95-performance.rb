require 'pathname'

test_name "Razor Server Performance Testing"

Razor = hosts('razor-server')

step "Upload the perftest tool source code"
source = (Pathname.new(__FILE__).dirname + "../perftest").cleanpath.to_s
on Razor, "rm -rf /tmp/perftest"
scp_to(Razor, source, "/tmp")

step "Install required packages to build"
on hosts('razor-server'), puppet_apply("--verbose"), :stdin => %q'
package { "build-essential":     ensure => installed }
package { "pkg-config":          ensure => installed }
package { "libglib2.0-dev":      ensure => installed }
package { "libcurl4-gnutls-dev": ensure => installed }
package { "liburiparser-dev":    ensure => installed }
package { "make":                ensure => installed }
'

step "Build perftest"
on Razor, "cd /tmp/perftest && make"

step "Collecting Razor image UUIDs"
ImageUUIDS = {}
on Razor, "razor image" do
  uuid  = nil
  stdout.split(/\n/).each do |line|
    match = /^\s*(\S.*)\s*=>\s*(\S.*)$/.match(line)
    next unless match and match[1] and match[2]

    key   = match[1].strip
    value = match[2].strip

    case key
    when 'UUID'
      uuid = value
    when 'ISO Filename'
      ImageUUIDS[value.downcase.sub(/\.iso$/, '')] = uuid
      uuid = nil
    end
  end
end


step "Ensure that Razor is running"
on Razor, "/opt/razor/bin/razor_daemon.rb status || /opt/razor/bin/razor_daemon.rb start"

step "Running perftest suite"
on Razor, "cd /tmp/perftest && " +
  "./perftest --target=localhost --esxi-uuid=#{ImageUUIDS['esxi']} " +
  "--ubuntu-uuid=#{ImageUUIDS['ubuntu']} --load=30 --population=5000"

step "Fetch back performance results"
Razor.each do |host|
  dir = Pathname('perf') + host
  dir.mkpath

  on host, "ls /tmp/perftest/*.{csv,jtl}", :acceptable_exit_codes => 0..65535 do
    stdout.split("\n").each do |file|
      next if file.include? '/*.' # nothing matches

      # This should be "scp_from", but that doesn't exist, so we live with
      # bloating our debug output with a copy of the data as well. :)
      on host, "cat #{file}" do
        (dir + Pathname(file).basename).open('wb') {|io| io.print stdout }
      end
    end
  end
end
