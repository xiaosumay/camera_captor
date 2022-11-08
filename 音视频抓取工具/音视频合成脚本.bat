@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
cd %~dp0


for /f "delims=" %%d in ('dir /ad /b /on') do (

	set dir_name=%%d
	set prefix="!dir_name:~0,1!"
	
	if !prefix! == "2" (
		echo dir_name is !dir_name!
		pushd %%d
		
		if not exist !dir_name!.mp4 (
			
			if exist "audio-1.pcm" (
				%~dp0ffmpeg.exe -loglevel error -y -f s16le -ar 16k -ac 1 -i audio-1.pcm audio-1.wav
			) else (
				%~dp0ffmpeg.exe -loglevel error -y -f s16le -ar 16k -ac 8 -i audio-8.pcm -map_channel 0.0.0 audio-1.wav
			)
			
			if exist "video.mp4" (
				%~dp0ffmpeg.exe -loglevel error -y -r 25 -i audio-1.wav -i video.mp4 -c:a aac -c:v copy -movflags +faststart -shortest !dir_name!.mp4
			) else (
				%~dp0ffmpeg.exe -loglevel error -y -r 25 -c:v mjpeg_qsv -i image-%%08d.jpg -i audio-1.wav !dir_name!.mp4
			)
		)
		
		popd
	)
)
ENDLOCAL

pause