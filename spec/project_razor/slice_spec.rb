require 'spec_helper'
require 'project_razor/slice'

describe ProjectRazor::Slice do
  # before :each do
  #   @test = TestClass.new
  #   @test.extend(ProjectRazor::SliceUtil::Common)
  #   # TODO: Review external dependencies here:
  #   @test.extend(ProjectRazor::Utility)
  # end

  context "code formerly known as SliceUtil::Common" do
    describe "get_web_args" do
      it "should return value for matching key" do
        ProjectRazor::Slice.new(['{"@k1":"v1","@k2":"v2","@k3":"v3"}']).
          get_web_vars(['k1', 'k2']).should == ['v1','v2']
      end

      it "should return nil element for nonmatching key" do
        ProjectRazor::Slice.new(['{"@k1":"v1","@k2":"v2","@k3":"v3"}']).
          get_web_vars(['k1', 'k4']).should == ['v1', nil]
      end

      it "should return nil for invalid JSON" do
        ProjectRazor::Slice.new(['\3"}']).get_web_vars(['k1', 'k2']).should == nil
      end
    end

    describe "get_cli_args" do
      TestArgs = [
        "template=debian_wheezy",
        "label=debian",
        "image_uuid=3RpS0x2KWmITuAsHALa3Ni"
      ]

      it "should return value for matching key" do
        ProjectRazor::Slice.new(TestArgs).get_cli_vars(['template', 'label']).
          should == ['debian_wheezy', 'debian']
      end

      it "should return nil element for nonmatching key" do
        ProjectRazor::Slice.new(TestArgs).get_cli_vars(['template', 'foo']).
          should == ['debian_wheezy', nil]
      end
    end

    describe "validate_arg" do
      subject('slice') { ProjectRazor::Slice.new([]) }
      it "should return false for empty values" do
        [ nil, {}, '', '{}', '{1}', ['', 1], [nil, 1], ['{}', 1] ].each do |val|
          slice.validate_arg(*[val].flatten).should == false
        end
      end

      it "should return valid value" do
        slice.validate_arg('foo','bar').should == ['foo', 'bar']
      end
    end

  end
end
