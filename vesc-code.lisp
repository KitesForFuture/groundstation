(define launch-line-length 180)
(define min-eight-line-length 200)
(define max-eight-line-length 250)

(define launch 0)
(define eight 1)
(define landing 2)
(define final-landing 3)
(define flightmode launch)

(define generatormode 0)
(define motormode 1)
(define mode generatormode)

(define counter 10)

(define max-reel-out-tension 10)
(define landing-request -1)

(uart-start 115200)

(define offset (get-dist))

(define arr (array-create 16))
(define array (array-create 16))

(loopwhile t
    (progn
        
        (define counter (- counter 1))
        (if (< counter 0) (define counter 0) 0)
        
        (define line-length (- 0 (- (get-dist) offset) ) )
        ; SEND line-length AND line tension(current)
        (bufset-f32 arr 0 314)
        (bufset-f32 arr 4 line-length)
        (bufset-f32 arr 8 flightmode)
        (bufset-f32 arr 12 314)
        (uart-write arr)
        
        ; RECEIVE tension-request from kite
        (bufclear array)
        (define num-bytes-read (uart-read array 16))
        (if (> num-bytes-read 15)
            (if (and (= (bufget-f32 array 0) 314) (= (bufget-f32 array 12) 314))
                ; received something, must be landing request
                (progn
                	(define flightmode final-landing)
                	(define mode motormode)
                )
            )
        )
        
        (if (= mode generatormode)
            (progn
                (if (= flightmode launch)
                    (set-brake 0.1) ; LAUNCHING
                    (set-brake 9) ; GENERATING max-reel-out-tension
                )
                
                ; SWITCHING TO REEL-IN
                (if (and (= flightmode eight) (> (get-current) -1) (= counter 0))
                    (progn
                        (define mode motormode)
                        (define counter 100)
                    )
                )
                
                (if (and (= flightmode launch) (> line-length launch-line-length))
                    (define flightmode eight)
                )
                
                (if (and (= flightmode eight) (> line-length max-eight-line-length))
                	(progn
                		(define flightmode landing)
                		(define mode motormode)
                	)
                )
            )
            
            (progn
                (progn
                    ; REEL-IN WITH LOW TENSION
                    (if (= flightmode eight)
                        (set-current 5)
                        ; flightmode = landing or final-landing
                        (if (> line-length 30)
                            (set-current 3)
                            (if (> line-length 0)
                                (progn
                                    (set-current (+ 1.5 (* 0.05 line-length)))
                                )
                            )
                        )
                    )
                    
                    (if (< line-length min-eight-line-length)
                    	(define flightmode eight)
                    )
                    
                    ; SWITCHING TO REEL-OUT
                    (if (and (= flightmode eight) (< (get-duty) 0.1) (= counter 0))
                        (progn
                            ;(set-current 0)
                            (define mode generatormode)
                            (define counter 10)
                        )
                    )
                )
            )
        )
        
        (sleep 0.005)
    )
)
