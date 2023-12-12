; ModuleID = 'EvaLLVM'
source_filename = "EvaLLVM"

@VERSION = global i32 44, align 4
@0 = private unnamed_addr constant [7 x i8] c"X: %d\0A\00", align 1

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 39, i32* %x, align 4
  %x1 = load i32, i32* %x, align 4
  %tmpcmp = icmp ne i32 %x1, 42
  br i1 %tmpcmp, label %then, label %else5

then:                                             ; preds = %entry
  %x2 = load i32, i32* %x, align 4
  %tmpcmp3 = icmp ugt i32 %x2, 42
  br i1 %tmpcmp3, label %then4, label %else

then4:                                            ; preds = %then
  store i32 300, i32* %x, align 4
  br label %ifend

else:                                             ; preds = %then
  store i32 200, i32* %x, align 4
  br label %ifend

ifend:                                            ; preds = %else, %then4
  %tmpif = phi i32 [ 300, %then4 ], [ 200, %else ]
  br label %ifend6

else5:                                            ; preds = %entry
  store i32 100, i32* %x, align 4
  br label %ifend6

ifend6:                                           ; preds = %else5, %ifend
  %tmpif7 = phi i32 [ %tmpif, %ifend ], [ 100, %else5 ]
  %x8 = load i32, i32* %x, align 4
  %0 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([7 x i8], [7 x i8]* @0, i32 0, i32 0), i32 %x8)
  ret i32 0
}
