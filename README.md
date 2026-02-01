# digiSample

convert WAV to high quality sample using hacked replay of famous game 'Le manoir de Mortevielle'

cpc-power link : https://www.cpc-power.com/index.php?page=detail&num=1349 (Amstrad version)

# Short doc

## playSampleRoutine

HL = sample address in memory
DE = length to play

## playSampleContinue

DE = length to play

You can use this function after a first CALL to playSampleRoutine, then position and current volume is backup in order to continue later


