test_name "Install Razor using Puppet"

step "install razor modules"
# Find the package, and ensure we only have one!
require 'pathname'
if (modules = Pathname.glob("#{ENV['WORKSPACE']}/pkg/puppetlabs-razor-*.tar.gz")).size == 1
  pkg = modules[0].basename
  puts "we found #{pkg} to install"
else
  puts "we found NOTHING to install!"
  fail_test "unable to proceed, multiple modules found: #{modules.join(", ")}"
end

on hosts('razor-server'), "rm -f /tmp/puppetlabs-razor-*.tar.gz"
scp_to(hosts('razor-server'), "#{ENV['WORKSPACE']}/pkg/#{pkg}", '/tmp')
on hosts('razor-server'), "puppet module install --force /tmp/#{pkg}"

module_list  = "puppet module list --color=false 2>&1"
on hosts('razor-server'), module_list

missing_deps = "#{module_list} | sed -ne '/Missing dependency/ s/^.*'\\''\\(.*\\)'\\''.*$/\1/ p'"
on hosts('razor-server'), missing_deps

on hosts('razor-server'), "#{missing_deps} | xargs -n1 puppet module install"

step "configure razor"
on hosts('razor-server'), puppet_apply("--verbose"), :stdin => %q'
class { sudo:
    config_file_replace => false,
}

class { razor:
  username  => razor,
  mk_source => "https://github.com/downloads/puppetlabs/Razor-Microkernel/rz_mk_prod-image.0.9.0.4.iso",
}
'

step "validate razor installation"
on hosts('razor-server'), "/opt/razor/bin/razor_daemon.rb status" do
  assert_match(/razor_daemon: running/, stdout)
end

step "install gems to run the test suite"
on hosts('razor-server'), puppet_apply("--verbose"), :stdin => %q'
  package { [rake, rspec, mocha, net-ssh]: ensure => installed, provider => gem }
  package { curl: ensure => installed }
'
