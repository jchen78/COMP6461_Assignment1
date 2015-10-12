COMP 6461
Fall 2015
Professor Javad Sadri
Submission date: 2015-10-12

Students:
Jun-Duo Chen (29721118)
Kaichen Zhang (40000160)

Notes:
- The server expects all transfers to occur from subdirectory "serverFiles" accessible from the executable's directory.
- The client will download files to subdirectory "clientFiles" accessible from the executable's directory.
- The logs will be recorded in the file "logfile.txt" in the directory of the executable. In the case where server and
  client executables are located in the same directory, the log file will contain the (interleaved) log output of both
  executables.
- All integration tests were performed with both client and server on the same machine. It was assumed that the sample
  code provided will correctly identify remote hosts, and therefore that functionality was unchanged and untested.