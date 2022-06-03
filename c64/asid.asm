; A-SID - C64 bandpass filter + LFO
;
; Copyright (C) 2022 Orastron srl unipersonale
;
; A-SID is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, version 3 of the License.
;
; A-SID is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with A-SID.  If not, see <http://www.gnu.org/licenses/>.
;
; File author: Stefano D'Angelo

!cpu 6502
!to "asid.prg",cbm

; memory map
; $fb,$fc: used for indirect Y addressing
; $0801-$1fff: BASIC code
; $2000-$2eff: machine language code
; $2f00-$2fff: machine code paddle read routine
; $3000-$3fff: lfo scaling data (16 sets of 256 elements)
; $4000-$43e7: colormap 1 (green)
; $4400-$47e7: colormap 2 (blue)
; $4800-$4be7: colormap 3 (violet)
; $4c00-$4fe7: colormap 4 (red)
; $6000-$7f3f: bitmap
; $7f40-$7f43: parameter colors
; $7f50-$7f5f: lfo phase increments
; $8000-$80ff: paddle mapping data (256 elements, set by boot program)
; $8100-$81ff: bar length in pixels, lookup data (256 elements)
; $8200-$83ff: cutoff to SID mapping lookup table (256 low byte entries followed by corresponding 256 high byte entries)
; $c000: cutoff frequency parameter (0-15)
; $c001: lfo amount parameter (0-15)
; $c002: lfo speed parameter (0-15)
; $c003: current parameter index (0-2)
; $c004: current number of update iterations in main loop
; $c005: number of iterations left to trigger left joystick event
; $c006: number of iterations left to trigger right joystick event
; $c008-$c00a: parameters for routines
; $c00c: current modulated cutoff value (0-255)
; $c00e: current colormap index
; $c00f: current parameter color
; $c010: lfo phase (0-255)
; $c011: lfo output (signed)
; $c014: 1 if paddle is present, 0 if not (set by boot program)
; $c015: paddle min value (set by boot program)
; $c016: paddle max value (set by boot program)
; $c017: mapped paddle value [-127,127]
; $c018: bar length in pixels
; $c019: number of bar lines left to draw
; $c01a: unmapped paddle value

* = $0801 ; BASIC code
!binary "boot.prg",,2

* = $2000 ; machine language code

.vic_setup:
	; set hi res mode
	lda #$3b
	sta $d011
	lda #$08
	sta $d016
	; set VIC bank
	lda $dd00
	and #%11111100
	ora #%00000010
	sta $dd00

.sid_setup:
	; full volume, bp filter mode
	lda #$2f
	sta $d418
	; res=15, external input to filter
	lda #$f8
	sta $d417

.initial_setup:
	lda #$f
	sta $c000			; default cutoff frequency = 15
	lda #$0
	sta $c001			; default lfo amount = 0
	lda #$7
	sta $c002			; default lfo speed = 7
	lda #$00
	sta $c003			; default parameter = 0 (cutoff)
	lda #$00
	sta $c010			; intiial lfo phase = 0
	; set 8 iterations left per left/right joystick events
	lda #$03
	sta $c005
	sta $c006

.main_loop:
	ldy $dc01			; read joystick port 1

.joystick_left:
	tya				; put joystick byte in A
	and #$04			; zero if left
	bne .joystick_left_reset	; if != 0 reset and skip
	dec $c005			; decrement event counter
	bne .joystick_right		; if != 0 skip
	lda $c003			; get current parameter index -> A
	beq .joystick_left_wrap		; if = 0, see below...
	sec
	sbc #$01			; if != 0, subtract 1
	jmp .joystick_left_end
.joystick_left_wrap:
	lda #$02			; ... wrap
.joystick_left_end:
	sta $c003			; save current parameter index
.joystick_left_reset:
	lda #$03
	sta $c005			; reset event counter

.joystick_right:
	tya				; put joystick byte in A
	and #$08			; zero if right
	bne .joystick_right_reset	; if != 0 reset and skip
	dec $c006			; decrement event counter
	bne .joystick_up		; if != 0 skip
	lda $c003			; get current parameter index -> A
	cmp #$02
	beq .joystick_right_wrap	; if = 2, see below...
	clc
	adc #$01			; if != 2, add 1
	jmp .joystick_right_end
.joystick_right_wrap:
	lda #$00			; ... wrap
.joystick_right_end:
	sta $c003			; save current parameter index
.joystick_right_reset:
	lda #$03
	sta $c006			; reset event counter

.joystick_up:
	tya				; put joystick byte in A
	and #$01			; zero if up
	bne .joystick_down		; if non-zero skip
	ldx $c003			; get current parameter index -> X
	lda $c000,X			; get current value -> A
	cmp #$f
	beq .joystick_down		; if max skip
	inc $c000,X			; increment parameter value

.joystick_down:
	tya				; put joystick byte in A
	and #$02			; zero if down
	bne .paddle			; if non-zero skip
	ldx $c003			; get current parameter index -> X
	lda $c000,X			; get current value
	beq .paddle			; if zero skip
	dec $c000,X			; decrement parameter value

.paddle:
	lda #$0
	sta $c017			; store 0 as paddle value
	lda $c014			; get paddle is present -> A
	beq .io_end			; if not present skip

	jsr $2f00			; call paddle read routine
	lda $c01a			; get paddle x value -> A

.paddle_min:
	cmp $c015			; compare with minimum
	bcs .paddle_max			; if >= minimum jump ahead
	lda $c015			; put minimum -> A
	jmp .paddle_map			; jump to mapping code
.paddle_max:
	cmp $c016			; compare with maximum
	bcc .paddle_map			; if < maximum jump ahead
	lda $c016			; put maximum -> A
.paddle_map:
	tax				; copy A -> X
	lda $8000,X			; get mapped value -> A
	sta $c017			; store mapped paddle value

.io_end:
	lda #$04
	sta $c004			; set 4 number of update iterations in main loop

.lfo:
	lda $c010			; get lfo phase -> A
	ldx $c002			; get lfo speed -> X
	clc
	adc $7f50,X			; add lfo phase increment
	sta $c010			; store lfo phase
	tay				; and also store it in Y
	lda $c001			; get lfo amount -> A
	clc
	adc #$30			; add #$30
	sta $fc				; and store result into $fc
	lda #$0
	sta $fb				; store 0 into $fb
	lda ($fb),Y			; get scaled lfo -> A
	sta $c011			; and store it into $c011

.cutoff_compute:
	lda $c000			; get cutoff frequency parameter -> A
	asl
	asl
	asl
	asl				; << 4
	ora #$08			; | 8
	tax				; copy A -> X

.cutoff_compute_paddle:
	lda $c017			; get mapped paddle value -> A
	cmp #$80			; compare with #$80
	bcs .cutoff_compute_paddle_neg	; if gte then paddle is negative and jump below
	txa				; copy X -> A
	clc
	adc $c017			; add paddle value
	bcc .cutoff_compute_paddle_end	; if no overflow jump below
	lda #$ff			; put #$ff -> A
	jmp .cutoff_compute_paddle_end	; jump below
.cutoff_compute_paddle_neg:
	txa				; copy X -> A
	clc
	adc $c017			; add paddle value
	bcs .cutoff_compute_paddle_end	; if overflow jump below
	lda #$00			; put 0 -> A
.cutoff_compute_paddle_end:
	tax				; copy A -> X

.cutoff_compute_lfo:
	lda $c011			; get lfo output -> A
	cmp #$80			; compare with #$80
	bcs .cutoff_compute_lfo_neg	; if gte then lfo is negative and jump below
	txa				; copy X -> A
	clc
	adc $c011			; add lfo output
	bcc .cutoff_compute_end		; if no overflow jump to end
	lda #$ff			; put #$ff -> A
	jmp .cutoff_compute_end		; jump to end
.cutoff_compute_lfo_neg:
	txa				; copy X -> A
	clc
	adc $c011			; add lfo output
	bcs .cutoff_compute_end		; if overflow jump to end
	lda #$00			; put 0 -> A

.cutoff_compute_end:
	sta $c00c			; put result in $c00c

.bar_draw:
	ldx $c00c			; get current modulated cutoff -> X
	lda $8100,X			; get bar length in pixels -> A
	sta $c018			; store value in $c018
	ldx #$60
	stx $fb
	ldx #$66
	stx $fc				; put upper left corner=$6660, in $fb, $fc
	lda #$78			; put 120 -> A
	sta $c019			; put A -> $c019

.bar_draw_clear_loop:
	cmp $c018			; compare with bar length
	beq .bar_draw_fill_loop		; if A = $c018 then jump below
	sec
	sbc #$8				; A - 8 -> A
	bcc .color_compute		; if overflow, done, move to next task
	cmp $c018			; compare with bar length
	sta $c019			; put number of lines left -> $c019
	bcc .bar_draw_mixed		; if A < $c018 then jump below

	ldy #$8				; put 8 -> Y
	lda #$40			; put $40 -> A
.bar_draw_clear_inner_loop:
	dey
	sta ($fb),y
	bne .bar_draw_clear_inner_loop	; loop

	; update memory pointer
	lda $fb				; get low byte of memory pointer -> A
	clc
	adc #$40			; add $40
	sta $fb				; store low byte
	lda $fc				; get high byte -> A
	adc #$01			; add 1 + carry
	sta $fc				; store high byte

	lda $c019			; get back number of lines left -> A
	jmp .bar_draw_clear_loop	; loop

.bar_draw_mixed:
	lda $c018			; put bar length -> A
	sec
	sbc $c019			; subtract number of lines left (-8)
	tax				; copy A -> X

	ldy #$8				; put 8 -> Y
	lda #$5f			; put $5f -> A
.bar_draw_mixed_fill_loop:
	dey
	dex
	sta ($fb),y
	bne .bar_draw_mixed_fill_loop	; loop

	lda #$40			; put $40 -> A
.bar_draw_mixed_clear_loop:
	dey
	sta ($fb),y
	bne .bar_draw_mixed_clear_loop	; loop

	; update memory pointer
	lda $fb				; get low byte of memory pointer -> A
	clc
	adc #$40			; add $40
	sta $fb				; store low byte
	lda $fc				; get high byte -> A
	adc #$01			; add 1 + carry
	sta $fc				; store high byte

.bar_draw_fill_loop:
	lda $c019			; get number of lines left -> A
	beq .color_compute		; if A = 0, then done, move to next task
	sec
	sbc #$8				; A - 8 -> A
	sta $c019			; put number of lines left -> $c019

	ldy #$8				; put 8 -> Y
	lda #$5f			; put $5f -> A
.bar_draw_fill_inner_loop:	
	dey
	sta ($fb),y
	bne .bar_draw_fill_inner_loop	; loop

	; update memory pointer
	lda $fb				; get low byte of memory pointer -> A
	clc
	adc #$40			; add $40
	sta $fb				; store low byte
	lda $fc				; get high byte -> A
	adc #$01			; add 1 + carry
	sta $fc				; store high byte

	lda $c019			; get back number of lines left -> A
	jmp .bar_draw_fill_loop		; loop

.color_compute:
	lda $c00c			; get current modulated cutoff -> A
	lsr
	lsr
	lsr
	lsr
	lsr
	lsr				; >> 6
	sta $c00e			; store current colormap index
	tax				; copy A -> X
	lda $7f40,X			; get current parameter color
	sta $c00f			; and store it

.cutoff_update:
	ldy $c00f			; parameter color -> Y
	lda $c003			; get current parameter index -> A
	beq .cutoff_update_color_ok	; if selected don't set to grey
	ldy #$b1			; grey color -> Y
.cutoff_update_color_ok:
	sty $c008			; set Y -> color
	lda #$00
	sta $c009			; set 0 -> x offset
	lda $c000			; get cutoff parameter value -> A
	sta $c00a			; set A -> parameter value
	jsr .param_update

.lfo_amount_update:
	ldy $c00f			; parameter color -> Y
	lda $c003			; get current parameter index -> A
	sec
	sbc #$01			; subtract 1
	beq .lfo_amount_update_color_ok	; if selected don't set to grey
	ldy #$b1			; grey color -> Y
.lfo_amount_update_color_ok:
	sty $c008			; set Y -> color
	lda #$09
	sta $c009			; set 9 -> x offset
	lda $c001			; get lfo amount parameter value -> A
	sta $c00a			; set A -> parameter value
	jsr .param_update

.lfo_speed_update:
	ldy $c00f			; parameter color -> Y
	lda $c003			; get current parameter index -> A
	sec
	sbc #$02			; subtract 2
	beq .lfo_speed_update_color_ok	; if selected don't set to grey
	ldy #$b1			; grey color -> Y
.lfo_speed_update_color_ok:
	sty $c008			; set Y -> color
	lda #$12
	sta $c009			; set 18 -> x offset
	lda $c002			; get lfo speed parameter value -> A
	sta $c00a			; set A -> parameter value
	jsr .param_update

.sid_update:
	ldx $c00c			; get current modulated cutoff -> X
	lda $8200,X			; get mapped low byte -> A
	sta $d415			; set low byte
	lda $8300,X			; get mapped high byte -> A
	sta $d416			; set high byte

; doing it here to get colormap updated before showing it (avoids giltches)
.vic_update:
	; set bitmap addresses
	lda $c00c			; get current modulated cutoff -> A
	lsr
	lsr				; >> 2
	ora #$08			; A | 8
	sta $d018			; set colormap address

.main_loop_end:
	dec $c004			; decrement number of update iterations
	beq .main_loop_loop		; if == 0 restart main loop (needed because far jump)
	jmp .lfo			; otherwise update again
.main_loop_loop:
	jmp .main_loop

; routine that updates colormap for parameter
; inputs:
;   $c008: color byte
;   $c009: x offset (number of cells)
;   $c00a: parameter value (0-15)
.param_update:
	; upper left corner of first parameter = 40a0/44a0/48a0/4ca0
	; here we put 40a0 + x offset into $fb, $fc for indirect addressing
	lda $c009	; put x offset -> A
	clc
	adc #$a0	; add low byte
	sta $fb		; store low byte in $fb
	lda $c00e	; get current colormap index -> A
	asl
	asl		; * 4
	adc #$40	; add 40 (with carry)
	sta $fc		; store high byte in $fc
	
	ldx #$15	; 21 lines left -> X

	; $c00a = 15 - $c00a (number of white cells)
	lda #$0f
	sec
	sbc $c00a
	sta $c00a

.param_update_loop:
	lda $c008		; get color byte -> A

	; put color data in 3 consecutive bytes
	ldy #$00		; put 0 -> Y
	sta ($fb),y		; 1
	iny
	sta ($fb),y		; 2
	iny
	sta ($fb),y		; 3
	iny

	; put color data or all white for middle cell
	cpx #$15
	beq .param_update_mcell	; if first line use color data
	lda $c00a		; get number of white cells -> A
	bne .param_update_mdec  ; if !=0 jump
	lda $c008		; get color byte -> A
	jmp .param_update_mcell	; jump
.param_update_mdec:
	dec $c00a		; decrement
	lda #$11		; use white
.param_update_mcell:
	sta ($fb),y		; 4
	iny

	lda $c008		; get color byte -> A

	; put color data in 3 consecutive bytes
	sta ($fb),y		; 5
	iny
	sta ($fb),y		; 6
	iny
	sta ($fb),y		; 7

	dex			; decrement lines left
	beq .param_update_end	; end loop if no lines left

	; update memory pointer
	lda $fb			; get low byte of memory pointer -> A
	clc
	adc #$28		; add 40
	sta $fb			; store low byte
	lda $fc			; get high byte -> A
	adc #$00		; add carry
	sta $fc			; store high byte

	jmp .param_update_loop	; loop

.param_update_end:
	rts		; return

; routines that reads paddle x value in port 2
; adapted from C64 programming reference, essentially a normal read with some waiting
* = $2f00
.paddle_read:
	sei				; disable interrupts
	lda $dc02			; get CIA 1 data direction port A
	tay				; copy it in Y
	lda #$c0
	sta $dc02			; put #$c0 -> CIA 1 data direction port A
	lda #$80
	sta $dc00			; put #$80 -> CIA 1 data port A (choose paddle in port 2)
	ldx #$80			; put #$80 -> X
.paddle_read_wait_loop:
	nop
	dex
	bpl .paddle_read_wait_loop	; loop
	lda $d419			; get value -> A
	sta $c01a			; store it in $c01a
	sty $dc02			; restore CIA 1 data direction port A
	cli				; re-enable interrupts
	rts

; lfo scaling
* = $3000
!binary "lfoscaling.dat"

; colormap 1 (green)
* = $4000
!binary "colormap1.dat"

; colormap 2 (blue)
* = $4400
!binary "colormap2.dat"

; colormap 3 (violet)
* = $4800
!binary "colormap3.dat"

; colormap 4 (red)
* = $4c00
!binary "colormap4.dat"

; bitmap
* = $6000
!binary "bitmap.dat"

; parameter colors
* = $7f40
!8 $51, $61, $41, $21

; lfo increments
* = $7f50
!8 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 16, 21, 28, 37, 49, 64

; bar mapping
* = $8100
!binary "barmapping.dat"

; cutoff mapping
* = $8200
!binary "cutoffmapping.dat"
