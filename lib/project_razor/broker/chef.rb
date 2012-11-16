# Our chef plugin which contains the agent & device proxy classes used for hand off

# TODO - Make broker properties open rather than rigid
require "erb"
require "net/ssh"

# Root namespace for ProjectRazor
module ProjectRazor::BrokerPlugin

  # Root namespace for Chef Broker plugin defined in ProjectRazor for node handoff
  class Chef < ProjectRazor::BrokerPlugin::Base
    include(ProjectRazor::Logging)

    def initialize(hash)
      super(hash)

      @plugin = :chef
      @description = "OpsCode Chef"
      @hidden = false
      from_hash(hash) if hash
    end

    def agent_hand_off(options = {})
      @options = options
      @options[:server] = @servers.first
      @options[:ca_server] = @options[:server]
      @options[:version] = @broker_version
      @options[:puppetagent_certname] ||= @options[:uuid].base62_decode.to_s(16)
      return false unless validate_options(@options, [:username, :password, :server, :ca_server, :puppetagent_certname, :ipaddress])
      @chef_script = compile_template
      init_agent(options)
    end

    def init_agent(options={})
      @run_script_str = ""
      begin
        Net::SSH.start(options[:ipaddress], options[:username], { :password => options[:password], :user_known_hosts_file => '/dev/null'} ) do |session|
          logger.debug "Copy: #{session.exec! "echo \"#{@chef_script}\" > /tmp/chef_init.sh" }"
          logger.debug "Chmod: #{session.exec! "chmod +x /tmp/chef_init.sh"}"
          @run_script_str << session.exec!("bash /tmp/chef_init.sh |& tee /tmp/chef_init.out")
          @run_script_str.split("\n").each do |line|
            logger.debug "chef script: #{line}"
          end
        end
      rescue => e
        logger.error "chef agent error: #{e}"
        return :broker_fail
      end
      # set return to fail by default
      ret = :broker_fail
      # set to wait
      ret = :broker_wait if @run_script_str.include?("Exiting; no certificate found and waitforcert is disabled")
      # set to success (this meant autosign was likely on)
      ret = :broker_success if @run_script_str.include?("Finished catalog run")
      ret
    end


    def compile_template
      logger.debug "Compiling template"
      install_script = File.join(File.dirname(__FILE__), "chef/agent_install.erb")
      contents = ERB.new(File.read(install_script)).result(binding)
      logger.debug("Compiled installation script:")
      logger.error install_script
      #contents.split("\n").each {|x| logger.error x}
      contents
    end

    def validate_options(options, req_list)
      missing_opts = req_list.select do |opt|
        options[opt] == nil
      end
      unless missing_opts.empty?
        false
      end
      true
    end
  end
end
