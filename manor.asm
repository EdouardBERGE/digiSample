
buildsna
bankset 0

org #38
ei : ret

org #100
run #100

ld sp,#100

; playing sample first run
ld bc,#7F10 : out (c),c : ld bc,#7F54 : out (c),c ; noir
ld hl,leSample
ld de,(laFin-leSample)>>4 ; play 1/16 of the sample
call playSampleRoutine

ld xl,15
split16
ld bc,#7F10 : out (c),c : ld a,r : and 31 : add #40 : out (c),a ; couleur aléatoire
; effacer un petit morceau de mémoire pour voir si la coupure s'entend
ld l,#C0
ld a,r : or #C0 : ld h,a
.randMem ld a,r : ld (hl),a : inc l : jr nz,.randMem
ld de,(laFin-leSample)>>4 ; play next 1/16
call playSampleContinue
dec xl
jr nz,split16

; done
jr $ ; infinite loop

;==================================
        playSampleContinue
;==================================
; call this with DE = length to play from last known position
di
exx
playSample_BCX ld bc,#1234
exx
playSample_HL ld hl,#1234
playSample_A ld a,#12
ld b,#F6
jr playSample

;==================================
        playSampleRoutine
;==================================
; ripped from MORTEVIELLE MANOR sample replay
; average sample rate is 71 nops => 15KHz replay
; it's useless to stabilize machine time for this
; HL=sample
; DE=longueur
di
exx
ld b,#F4 ; port PPI
ld c,#0C ; valeur de travail ET de départ du volume
ld a,#09
out (c),a ; sélection du registre de volume central
exx
ld bc,#F6C0 : out (c),c : out (c),0 ; validation du registre de volume

playSample
ld c,(hl)
rebique ld a,c : rrca : rrca : rrca : rrca : and 7
exx
	ld hl,deltaSampleConversion
	add l : ld l,a : ld a,c : add (hl) : ld c,a
	out (c),a ; envoyer le volume
exx
ld a,#80 : out (c),a : out (c),0
ld a,6 : dec a : jp nz,$-1 ; tempo
bit 7,c ; tester la répétition du delta
jp nz,ajusteSample
dec de
inc hl
cp (hl) : cp (hl) : cp (hl) : bit 3,a ; tempo

rebiqueBis
ld a,c : and 7
exx
	ld hl,deltaSampleConversion
	add l : ld l,a : ld a,c : add (hl) : ld c,a
	out (c),a ; envoyer le volume
exx
ld a,#80 : out (c),a : out (c),0
ld a,6 : dec a : jp nz,$-1 ; tempo
bit 3,c ; tester la répétition du delta
jr nz,ajusteSampleBis
ld a,e : or a : jp nz,playSample
ld a,d : or a : jp nz,playSample

; store data in case we want to go further in the sample
ld (playSample_HL+1),hl ; current data sample
ld (playSample_A+1),a ; current volume
exx
ld (playSample_BCX+1),bc
ei
ret

copyRight ld l,l : ld h,c : ld l,(hl) : ld l,a : ld (hl),d ;)

ajusteSample
res 7,c ; reset repetition bit
jp rebique
ajusteSampleBis
res 3,c ; reset repetition bit
cp (hl) : cp (hl) : ld a,r ; original tempo
jp rebiqueBis


confine 8
deltaSampleConversion
defb #fc,#fe,#ff,0,1,2,4,8 ; -4,-2,-1,0,...

leSample
incbin 'output.bin'
laFin


