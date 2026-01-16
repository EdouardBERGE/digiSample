
buildsna
bankset 0

org #100
run #100
ld hl,leSample
ld de,laFin-leSample
call playSampleRoutine
jr $

playSampleRoutine
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


