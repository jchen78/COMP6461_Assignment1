COMP 6461
Fall 2015
Professor Javad Sadri
Submission date: 2015-11-24

Students:
Jun-Duo Chen (29721118)
Kaichen Zhang (40000160)

Notes:
- The server expects all transfers to occur from subdirectory "serverFiles" accessible from the executable's directory.
- The client will download files to subdirectory "clientFiles" accessible from the executable's directory.
- It is the responsibility of the operator to ensure that the file selected for PUT is valid -- there is no error checking
  for invalid filename input.
- The logs will be recorded in the file "logfile.txt" in the directory of the executable. In the case where server and
  client executables are located in the same directory, the log file will contain the (interleaved) log output of both
  executables. The client multiplexer has a much more verbose logging functionality, and as such will save to muxerLog.txt
  instead.
- All integration tests were performed with both client and server on the same machine. It was assumed that the sample
  code provided will correctly identify remote hosts, and therefore that functionality was unchanged and untested.
- To run the application, the following projects must be started in the specified order:
	- 1: UdpServer
	- 2: Router
	- 3: UdpClientMuxer
	- 4: UdpClient