@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
cd %~dp0


for /f "delims=" %%d in ('dir /ad /b /on') do (

	set dir_name=%%d
	set prefix="!dir_name:~0,2!"
	
	if !prefix! == "22" (
		echo dir_name is !dir_name!
		pushd %%d
		
		if exist "audio-1.pcm" (
			%~dp0ffmpeg.exe -loglevel error -y -f s16le -ar 16k -ac 1 -i audio-1.pcm audio-1.wav
		) else (
			%~dp0ffmpeg.exe -loglevel error -y -f s16le -ar 16k -ac 8 -i audio-8.pcm -map_channel 0.0.0 audio-1.wav
		)
		
		%~dp0ffmpeg.exe -loglevel error -y -r 25 -i video.mp4 -i audio-1.wav !dir_name!.mp4
		
		popd
	)
)

ENDLOCAL