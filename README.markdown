# Project Razor

[![Build Status](https://jenkins.puppetlabs.com/job/Razor%20Acceptance/badge/icon)](https://jenkins.puppetlabs.com/job/Razor%20Acceptance/)

## Introduction

Project Razor is a power control, provisioning, and management application
designed to deploy both bare-metal and virtual computer resources. Razor
provides broker plugins for integration with third party such as Puppet.

This is a 0.x release, so the CLI and API is still in flux and may change. Make sure you __read the release notes before upgrading__

## How to Get Help

We really want Razor to be simple to contribute to, and to ensure that you can
get started quickly.  A big part of that is being available to help you figure
out the right way to solve a problem, and to make sure you get up to
speed quickly.

You can always reach out and ask for help:

* by email or through the web on the [puppet-razor@googlegroups.com][puppet-razor]
  mailing list.  (membership is required to post.)
* by IRC, through [#puppet-razor][irc] on [freenode][freenode].

[puppet-razor]: https://groups.google.com/forum/?fromgroups#!forum/puppet-razor
[irc]:            irc://chat.freenode.net/puppet-razor
[freenode]:   http://freenode.net/

## Authors

* [Nicholas Weaver](https://github.com/lynxbat)
* [Tom McSweeney](https://github.com/tjmcs)

## Installation

* Razor Overview: [Nickapedia.com](http://nickapedia.com/2012/05/21/lex-parsimoniae-cloud-provisioning-with-a-razor)
* Razor Session from PuppetConf 2012: [Youtube](http://www.youtube.com/watch?v=cR1bOg0IU5U)

Follow wiki documentation for installation process:

https://github.com/puppetlabs/Razor/wiki/installation

## Razor MicroKernel
* The Razor MicroKernel project:
[https://github.com/puppetlabs/Razor-Microkernel](https://github.com/puppetlabs/Razor-Microkernel)
* The Razor MK images are officially available at:
[https://github.com/puppetlabs/Razor-Microkernel/downloads](https://github.com/puppetlabs/Razor-Microkernel/downloads)

## Environment Variables
* $RAZOR\_HOME: Razor installation root directory.
* $RAZOR\_RSPEC\_WEBPATH: _optional_ rspec HTML output path.
* $RAZOR\_LOG\_PATH: _optional_ Razor log directory (default: ${RAZOR_HOME}/log).
* $RAZOR\_LOG\_LEVEL: _optional_ Razor log output verbosity level:

        0 = Debug
        1 = Info
        2 = Warn
        3 = Error (default)
        4 = Fatal
        5 = Unknown

## Directory structure
    ./bin - control scripts
    ./conf - configuration YAML files
    ./doc - Documentation (YARD)
    ./images - default images directory
    ./install - installation bits
    ./lib - root library folder
    ./test_scripts - testing scripts
    ./rspec - unit tests (RSpec)

## Starting services

Start Razor API with:

    cd $RAZOR_HOME/bin
    ./razor_daemon.rb start

## License

See LICENSE file.

## Reference

* Razor Overview: [Nickapedia.com](http://nickapedia.com/2012/05/21/lex-parsimoniae-cloud-provisioning-with-a-razor)
* Puppet Labs Razor Module:[Puppetlabs.com](http://puppetlabs.com/blog/introducing-razor-a-next-generation-provisioning-solution/)
