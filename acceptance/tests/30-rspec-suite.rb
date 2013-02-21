test_name "RSpec based integration tests"
on hosts('razor-server'), "cd /opt/razor && rspec -I acceptance-spec -fd -c acceptance-spec"
