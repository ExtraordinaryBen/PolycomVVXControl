# PolycomVVXControl
A command line utility for remote controlling Polycom VVX IP phones

# Description
	Application to remote control Polycom VVX IP phones via their web interface (using HTTP).
	This application is initially intended to perform certain actions on phones running in
	Microsoft Lync mode. These actions include:
		* get device information
		* get status
		* sign in using PIN authentication
		* sign out
		* reboot
		* factory reset
	It also supports performing actions in batch reading data from a CSV file.

# License
	This program is distributed under the GNU Lesser General Public License v2.1 (see the file COPYING)
	It makes use libraries which have their individual licenses:
		* libcurl       MIT (or Modified BSD)-style license
		* libcsv        GNU Library or Lesser General Public License version 2.0 (LGPLv2)
		* expat	        MIT license

# Usage
	Run the following command to get the command line help:
		PolycomVVXControl.exe -h
