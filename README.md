# linux-automatic_marking_program
> This is program that automatic mark students .txt and .c files compare with answer.

  - It contains an answer directory (ANS_DIR hear).
  - It contains a students directory (STD_DIR hear) and they contain each students' answer directory (20190001, 20190002, ... here).
  - When you run this program, you can make 'score.csv' report card that contains students score.


## How It Works

###Linux & Mac:

 1. compile
```sh
gcc ssu_score.c -o ssu_score
```

 1. see usage
 ```sh
 ./ssu_score -h
```

 - with no options: mark and make 'score.csv' report card.
```sh
./ssu_score STD_DIR ANS_DIR
```

 - with `-p` options: mark and print students' score and total average.
```sh
./ssu_score STD_DIR ANS_DIR -p
```

 - with `e` options: mark and print error on 'DIRNAME/ID/question_error.txt' file
```sh
./ssu_score STD_DIR ANS_DIR -e <DIRNAME>
```

