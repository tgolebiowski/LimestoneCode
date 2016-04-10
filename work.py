import subprocess;

#vcvarsallFilePath = "C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\vcvarsall.bat";
vcCompilerFilePath = "C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\bin\\cl.exe";

print "Work time"
compilerChildProcess = subprocess.Popen( vcCompilerFilePath, -1, None, None, None, None, None, False, True );
compilerChildProcess.wait();
print "compiler exitted with return code: "; 
print compilerChildProcess.returncode;


print "Input something to end"
raw_input();