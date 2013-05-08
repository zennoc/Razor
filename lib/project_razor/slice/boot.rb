require "json"

# Root ProjectRazor namespace
module ProjectRazor
  class Slice

    # ProjectRazor Slice Node (NEW)
    # Used for policy management
    class Boot < ProjectRazor::Slice
      # @param [Array] args
      def initialize(args)
        super(args)
        @hidden = true
        @engine = ProjectRazor::Engine.instance
      end

      def slice_commands
        { :boot    => "boot_call",
          :default => :boot,
          :else    => :boot }
      end

      def boot_call
        @command = :boot_call

        # This is only REST API.
        raise ProjectRazor::Error::Slice::NotImplemented, "Not implemented for CLI." unless @web_command

        begin
          # Grab next arg as json string var
          json_string = @command_array.first
          @vars_hash = sanitize_hash(JSON.parse(json_string))

          if @vars_hash['mac']
            @vars_hash['hw_id'] = @vars_hash['mac']
          end
          @hw_id = @vars_hash['hw_id']
          @dhcp_mac= @vars_hash['dhcp_mac'] || nil
        rescue JSON::ParserError => e
          error_reboot_node "Bad JSON #{e.inspect}"
          return
        end

        raise ProjectRazor::Error::Slice::MissingArgument, "Must Provide Hardware IDs[hw_id]" unless validate_arg(@hw_id)

        @hw_id = @hw_id.split("_") unless @hw_id.is_a? Array
        unless @hw_id.count > 0
          error_reboot_node "Must Provide At Least One Hardware ID [hw_id]"
          return
        end

        @hw_id.collect! {|x| x.upcase.gsub(':', '') }
        logger.info "Boot called by Node (HW_ID: #@hw_id)"
        logger.info "Calling Engine for boot script"
        puts @engine.boot_checkin(:hw_id => @hw_id, :dhcp_mac => @dhcp_mac)
      end

      def error_reboot_node(msg)
        puts "#{msg}\necho API Error, will reboot in 30 seconds\nsleep 30\nreboot\n"
      end
    end
  end
end
