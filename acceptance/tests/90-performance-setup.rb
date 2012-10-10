test_name "Razor Server Performance Testing Setup"

Razor = hosts('razor-server')

step "Flush the existing project_razor mongo database"
on Razor, "mongo project_razor --eval 'db.dropDatabase()'"

step "Restart the razor daemon"
on Razor, "/opt/razor/bin/razor_daemon.rb restart"


def add_image(args = {})
  what = args[:name] || args[:type]
  uuid = nil
  url  = args.delete(:url)
  iso  = "/tmp/#{what}.iso"
  args = args.map{|k, v| "--#{k} '#{v}'"}.join(' ')

  step "Fetch the #{what} ISO"
  on Razor, "curl -Lo #{iso} #{url}"

  step "Install the #{what} image"
  on Razor, "razor image add #{args} --path #{iso}" do
    uuid = (/UUID => +([a-zA-Z0-9]+)$/.match(stdout) || [])[1]
    uuid.length < 22 and fail_test("unable to match the #{what} UUID from Razor")
  end

  step "Remove the ISO image"
  on Razor, "rm -f #{iso}"

  return uuid
end

exi    = add_image(:type => 'esxi', :url  => "http://faro.puppetlabs.lan/Software/VMware/VMware-VMvisor-Installer-5.0.0-469512.x86_64.iso")
ubuntu = add_image(:type => 'os', :name => 'ubuntu', :version => '1204', :url => "http://faro.puppetlabs.lan/ISO/Ubuntu/ubuntu-12.04-server-amd64.iso")
centos = add_image(:type => 'os', :name => 'centos', :version => '62', :url => "http://faro.puppetlabs.lan/ISO/CentOS/CentOS-6.2-x86_64-minimal.iso")


