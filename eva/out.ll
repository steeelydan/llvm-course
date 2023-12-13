; ModuleID = 'EvaLLVM'
source_filename = "EvaLLVM"

@VERSION = global i32 44, align 4
@0 = private unnamed_addr constant [31 x i8] c"\0A(+ (square 2) (sum 2 3)): %d\0A\00", align 1

declare i32 @printf(i8*, ...)

define i32 @main() {
entry:
  %0 = call i32 @square(i32 2)
  %1 = call i32 @sum(i32 2, i32 3)
  %2 = call i32 @square(i32 2)
  %3 = call i32 @sum(i32 2, i32 3)
  %tmpadd = add i32 %2, %3
  %4 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([31 x i8], [31 x i8]* @0, i32 0, i32 0), i32 %tmpadd)
  ret i32 0
}

define i32 @square(i32 %x) {
entry:
  %x1 = alloca i32, align 4
  store i32 %x, i32* %x1, align 4
  %x2 = load i32, i32* %x1, align 4
  %x3 = load i32, i32* %x1, align 4
  %tmpmul = mul i32 %x2, %x3
  ret i32 %tmpmul
}

define i32 @sum(i32 %a, i32 %b) {
entry:
  %a1 = alloca i32, align 4
  store i32 %a, i32* %a1, align 4
  %b2 = alloca i32, align 4
  store i32 %b, i32* %b2, align 4
  %a3 = load i32, i32* %a1, align 4
  %b4 = load i32, i32* %b2, align 4
  %tmpadd = add i32 %a3, %b4
  ret i32 %tmpadd
}
