#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

#include "jun_runtime.h"

#define BUFFER_SIZE 1024
#define CORRECT 0
#define NO_FILE 1
#define MAKE_ERROR 2
#define INCORRECT 3
#define WARN_SCORE -0.1
#define ERROR_SCORE 0
#define TOO_MUCH_TIME 5

char std_dir_name[20] = "";
char ans_dir_name[20] = "";
char err_dir_name[20] = "error";
char sco_csv_path[10] = "score.csv";
int sco_csv_fd;
char t_name[5];
char t_name2[5];
int pb_count;
int pb_idx;
int pt_count;
int st_count;
int st_idx;
int warn_cnt;

struct student {
    char name[10];
    float score[105];
};

struct answer {
    char num[10];
    char type[5];
    float score;
    int flag_t;
};

struct student std[105];
struct answer ans[105];
char t_pb[5][10];
char c_pb[5][10];

void print_usage();
int filter(const struct dirent *dirent);
int filter_2(const struct dirent *dirent);
void erase_space(char buf[]);
int stricmp(char *cmp1, char *cmp2);
int is_identifier(char buf);
int grading_blank(int st_idx, int pb_idx);
int grading_program(int st_idx, int pb_idx);

int main(int argc, char *argv[])
{
    
    struct timeval begin_t, end_t;
    int flag_e = 0, flag_p = 0, flag_t = 0, flag_c = 0;
    int opt;

    int score_table_fd;
    char score_table_path[40];

    gettimeofday(&begin_t, NULL);

    /* case문 들어가기 전의 작업 */
    if ((argc == 1) || (argc == 2)){
        print_usage();
        exit(0);
    }
    else {
        if ((strchr(argv[1], '-') == NULL) && (strchr(argv[1], '-') == NULL)) {
            memcpy(std_dir_name, argv[1], 20);
            memcpy(ans_dir_name, argv[2], 20);
        }
        else if (strcmp(strchr(argv[1], '-'), "-c") == 0) {
        }
        else {
            print_usage();
            exit(0);
        }
    }

    // option 처리
    int e_optind, p_optind, t_optind, c_optind;
    char *option = NULL;
    struct get_option{
        char gopt;
        int goptind;
    };
    struct get_option my_option[5];
    int go = 0;
    optind = 1;
    while (argc > optind) {
        option = strchr(argv[optind], '-');
        if (option == NULL) {
            optind++;
            continue;
        }
        else {
            opt = (int)option[1];
        }
        switch (opt) {
            case 'e':
                my_option[go].gopt = 'e';
                my_option[go].goptind = optind;
                go++;
                flag_e = 1;
                break;
            case 'p':
                my_option[go].gopt = 'p';
                my_option[go].goptind = optind;
                go++;
                flag_p = 1;
                break;
            case 't':
                my_option[go].gopt = 't';
                my_option[go].goptind = optind;
                go++;
                flag_t = 1;
                break;
            case 'c':
                my_option[go].gopt = 'c';
                my_option[go].goptind = optind;
                go++;
                flag_c = 1;
                break;
            case 'h':
                print_usage();
                exit(0);
                break;
            default:
                print_usage();
                exit(0);
                break;
        }
        optind++;
    }
    my_option[go].gopt = 'f';
    my_option[go].goptind = argc;
    // option의 인자 받기
    int t_optarg, c_optarg;
    for (int i = 0; i < go; i++) {
        if (my_option[i].gopt == 'e') {
            if ((my_option[i+1].goptind - my_option[i].goptind) != 2) {
                print_usage();
                exit(0);
            }
            else
                memcpy(err_dir_name, argv[my_option[i].goptind + 1], 20);
        }
        if (my_option[i].gopt == 't') {
            if ((t_optarg = my_option[i+1].goptind - my_option[i].goptind - 1) < 1) {
                print_usage();
                exit(0);
            }
            else {
                if (t_optarg > 5) {
                    printf("Maximum Number of Argument Exceeded.  :: ");
                    for (int j = 6; j <= t_optarg; j++) {
                        printf("%s ", argv[my_option[i].goptind + j]);
                    }
                    printf("\n");
                }
                for (int j = 1; j < 6; j++) {
                    strcpy(t_pb[j-1], argv[my_option[i].goptind + j]);
                }
            }
        }
        if (my_option[i].gopt == 'c') {
            if ((c_optarg = my_option[i+1].goptind - my_option[i].goptind - 1) < 1) {
                print_usage();
                exit(0);
            }
            else {
                if (c_optarg > 5) {
                    printf("Maximum Number of Argument Exceeded.  :: ");
                    for (int j = 6; j <= c_optarg; j++) {
                        printf("%s ", argv[my_option[i].goptind + j]);
                    }
                    printf("\n");
                }
                for (int j = 1; j < 6; j++) {
                    strcpy(c_pb[j-1], argv[my_option[i].goptind + j]);
                }
            }
        }
    }

    // STD_DIR, ANS_DIR 없이 -c option을 받았을 경우
    if ((strcmp(std_dir_name, "") == 0) || (strcmp(ans_dir_name, "") == 0)) {
        if (flag_c == 1) {
            if (access(sco_csv_path, F_OK) < 0) {
                fprintf(stderr, "%s is doesn't exits.\n", sco_csv_path);
                exit(1);
            }
            else {
                printf("just c option\n");
                exit(0);
            }
        }
        else {
            print_usage();
            exit(0);
        }
    }

    /* 문제 번호 읽어오는 작업 */
    struct dirent **pb_namelist;

    if((pb_count = scandir(ans_dir_name, &pb_namelist, *filter, alphasort)) == -1) {
        fprintf(stderr, "%s Directory Scan Error\n", ans_dir_name);
        exit(1);
    }

    for (int i = 0; i < pb_count-1; i++) 
        for (int j = i+1; j < pb_count; j++){
            char tmp[1024];
            int a, b;
            a = atoi(pb_namelist[i]->d_name);
            b = atoi(pb_namelist[j]->d_name);
            if (a > b){
                strcpy(tmp, pb_namelist[i]->d_name);
                strcpy(pb_namelist[i]->d_name, pb_namelist[j]->d_name);
                strcpy(pb_namelist[j]->d_name, tmp);
            }
        }

    for (pb_idx = 0; pb_idx < pb_count; pb_idx++)
        strcpy(ans[pb_idx].num, pb_namelist[pb_idx]->d_name);

    /* 문제 형식 읽어오는 작업 */    
    struct dirent **pt_namelist;

    if (chdir(ans_dir_name) < 0) {
        fprintf(stderr, "chdir error\n");
        exit(1);
    }
    //문제 형식 읽기
    for (pb_idx = 0; pb_idx < pb_count; pb_idx++) {
        if((pt_count = scandir(pb_namelist[pb_idx]->d_name, &pt_namelist, *filter_2, NULL)) == -1) {
            fprintf(stderr, "%s Directory Scan Error\n", pb_namelist[pb_idx]->d_name);
            exit(1);
        }
        strcpy(pb_namelist[pb_idx]->d_name, pt_namelist[0]->d_name);
    }
    char *fil;
    for (pb_idx = 0; pb_idx < pb_count; pb_idx++){
        fil = strchr(pb_namelist[pb_idx]->d_name, '.');
        if (strcmp(fil, ".txt") == 0) 
            strcpy(ans[pb_idx].type, ".txt");
        else if (strcmp(fil, ".c") == 0)
            strcpy(ans[pb_idx].type, ".c");
    }

    //프로그램 파일 실행시켜 두기
    int pb_fd, tmp_fd;
    for (pb_idx = 45; pb_idx < pb_count; pb_idx++) {
        char line[25] = "";
        char pb_stdout[10] = "";
        if (strcmp(ans[pb_idx].type, ".c") == 0) {
            if (chdir(ans[pb_idx].num) < 0) {
                fprintf(stderr, "chdir error\n");
                exit(1);
            }
            strcat(line, "gcc ");
            strcat(line, ans[pb_idx].num);
            strcat(line, ans[pb_idx].type);
            strcat(line, " -o ");
            strcat(line, ans[pb_idx].num);
            strcat(line, ".exe");
            if (system(line) < 0) {
                fprintf(stderr, "compile error for %s%s\n", ans[pb_idx].num, ans[pb_idx].type);
                exit(1);
            }
            strcat(pb_stdout, ans[pb_idx].num);
            strcat(pb_stdout, ".stdout");
            if ((pb_fd = open(pb_stdout, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
                fprintf(stderr, "open error foe %s\n", pb_stdout);
                exit(1);
            }
            tmp_fd = dup(1);
            dup2(pb_fd, 1);
            strcpy(line, "./");
            strcat(line, ans[pb_idx].num);
            strcat(line, ".exe");
            system(line);
            dup2(tmp_fd, 1);
            close(pb_fd);
            if (chdir("../") < 0) {
                fprintf(stderr, "chdir error\n");
                exit(1);
            }
        }

    }
    if (chdir("../") < 0) {
        fprintf(stderr, "chdir error\n");
        exit(1);
    }

    // -t option 적용 시켜주기
    if (flag_t == 1) {
        int cnt = 0;
        if (t_optarg > 6)
            cnt = 5;
        else
            cnt = t_optarg;
        for (int i = 0; i < cnt; i++)
            for (pb_idx = 0; pb_idx < pb_count; pb_idx++) 
                if (strcmp(ans[pb_idx].num, t_pb[i]) == 0) {
                    ans[pb_idx].flag_t = 1;
                }
    }

    /* 점수 테이블 경로 */    
    memcpy(score_table_path, ans_dir_name, 40);
    strcat(score_table_path, "/score_table.csv");

    /* 점수 테이블 존재 여부 확인 */
    score_table_fd = open(score_table_path, O_WRONLY | O_CREAT | O_EXCL);

    /* 점수 테이블 존재*/
    if ((score_table_fd < 0)) {
        int line_size = 0;
        char score_buf[1];
        char score_line[10];
        int i = 0;
        if ((score_table_fd = open(score_table_path, O_RDONLY)) < 0) {
            fprintf(stderr, "open error for score_table.csv\n");
            exit(1);
        }
        for (pb_idx = 0; pb_idx < pb_count; pb_idx++) {
            if (strcmp(ans[pb_idx].type, ".txt") == 0) {
                line_size = strlen(ans[pb_idx].num);
                line_size += 5;
                lseek(score_table_fd, line_size, SEEK_CUR);
                while (1) {
                    read(score_table_fd, &score_buf, 1);
                    if (score_buf[0] == '\n') {
                        break;
                    }
                    else {
                        score_line[i++] = score_buf[0];
                    }
                }
                ans[pb_idx].score = atof(score_line);
                i = 0;
                strcpy(score_line, "");
            }
            else if (strcmp(ans[pb_idx].type, ".c") == 0) {
                line_size = strlen(ans[pb_idx].num);
                line_size += 3;
                lseek(score_table_fd, line_size, SEEK_CUR);
                while (1) {
                    read(score_table_fd, &score_buf, 1);
                    if (score_buf[0] == '\n') {
                        break;
                    }
                    else {
                        score_line[i++] = score_buf[0];
                    }
                }
                ans[pb_idx].score = atof(score_line);
                i = 0;
                strcpy(score_line, "");
            }
        }
    }
    /* 점수 테이블 없음 */
    else{
        int score_input_type;
        float blank_score;
        float program_score;
        char line[20] = "";
        char score_s[10];
        char *fil;

        printf("score_table.csv file doesn't exist in %s!\n", ans_dir_name);
        printf("1. input blank question and program question's score. ex) 0.5 1\n");
        printf("2. input all question's score. ex) Input value of 1-1: 0.1\n");
        printf("select type >> ");
        scanf("%d", &score_input_type);

        if (score_input_type == 1) {
            printf("Input value of blank question : ");
            scanf("%f", &blank_score);
            printf("Input value of program question : ");
            scanf("%f", &program_score);

            for (pb_idx = 0; pb_idx < pb_count; pb_idx++){
                if (strcmp(ans[pb_idx].type, ".txt") == 0) {
                    ans[pb_idx].score = blank_score;
                    strcat(line, ans[pb_idx].num);
                    strcat(line, ans[pb_idx].type);
                    strcat(line, ",");
                    sprintf(score_s, "%.2f", blank_score);
                    strcat(line, score_s);
                    strcat(line, "\n");

                    write(score_table_fd, line, strlen(line));
                    strcpy(line, "");
                }
                else if (strcmp(ans[pb_idx].type, ".c") == 0){
                    ans[pb_idx].score = program_score;
                    strcat(line, ans[pb_idx].num);
                    strcat(line, ans[pb_idx].type);
                    strcat(line, ",");
                    sprintf(score_s, "%.2f", program_score);
                    strcat(line, score_s);
                    strcat(line, "\n");

                    write(score_table_fd, line, strlen(line));
                    strcpy(line, "");
                }
            }
            close(score_table_fd);
        }
        else if (score_input_type == 2) {
            for (pb_idx = 0; pb_idx < pb_count; pb_idx++) {
                printf("Input of %s: ", pb_namelist[pb_idx]->d_name);
                scanf("%f", &blank_score);
                if (strcmp(ans[pb_idx].type, ".txt") == 0) {
                    ans[pb_idx].score = blank_score;
                    strcat(line, ans[pb_idx].num);
                    strcat(line, ans[pb_idx].type);
                    strcat(line, ",");
                    sprintf(score_s, "%.2f", blank_score);
                    strcat(line, score_s);
                    strcat(line, "\n");

                    write(score_table_fd, line, strlen(line));
                    strcpy(line, "");
                }
                else if (strcmp(ans[pb_idx].type, ".c") == 0){
                    ans[pb_idx].score = blank_score;
                    strcat(line, ans[pb_idx].num);
                    strcat(line, ans[pb_idx].type);
                    strcat(line, ",");
                    sprintf(score_s, "%.2f", program_score);
                    strcat(line, score_s);
                    strcat(line, "\n");

                    write(score_table_fd, line, strlen(line));
                    strcpy(line, "");
                }
            }
        }
        else {
            fprintf(stderr, "Selected type is wrong. Please give 1 or 2!\n");
            exit(0);
        }

    }

    /* score.csv 존재 확인 */
    if (access(sco_csv_path, F_OK) == 0) {
        char rm_dir[30] = "rm -rf ";
        strcat(rm_dir, sco_csv_path);
        system(rm_dir);
    }
    if ((sco_csv_fd = open(sco_csv_path, O_WRONLY | O_CREAT, 0666)) < 0) {
        fprintf(stderr, "open error for %s\n", sco_csv_path);
        exit(1);
    }
    else {
        // 첫번째 줄 채우기
        char pb_name[15] = "";
        lseek(sco_csv_fd, 0, SEEK_SET);
        write(sco_csv_fd, ",", 1);
        for (int pb_idx = 0; pb_idx < pb_count; pb_idx++) {
            strcat(pb_name, ans[pb_idx].num);
            strcat(pb_name, ans[pb_idx].type);
            write(sco_csv_fd, pb_name, strlen(pb_name));
            write(sco_csv_fd, ",", 1);
            strcpy(pb_name, "");
        }
        write(sco_csv_fd, "sum\n", 4);
    }

    /* error directory 생성*/
    if (access(err_dir_name, F_OK) == 0){ 
        char rm_dir[30] = "rm -rf ";
        strcat(rm_dir, err_dir_name);
        system(rm_dir);
    }
    mkdir(err_dir_name, 0777);

    /* 학생 파일 채점*/
    printf("grading student's test papers..\n");
    struct dirent **st_list;

    if((st_count = scandir(std_dir_name, &st_list, *filter, alphasort)) == -1) {
        fprintf(stderr, "%s Directory Scan Error\n", std_dir_name);
        exit(1);
    }
    for (st_idx = 0; st_idx < st_count; st_idx++)
        strcpy(std[st_idx].name, st_list[st_idx]->d_name);

    float all_score = 0;
    for (st_idx = 0; st_idx < st_count; st_idx++) {
        float sum = 0;
        int grd_res;
        float grd_sco;
        char grd_sco_s[10];
        // score.csv에 학번 써주기
        write(sco_csv_fd, std[st_idx].name, strlen(std[st_idx].name));
        write(sco_csv_fd, ",", 1);
        // 본격적으로 채점 함수 실행
        for (pb_idx = 0; pb_idx < pb_count; pb_idx++){
            if (strcmp(ans[pb_idx].type, ".txt") == 0) {
                grd_res = grading_blank(st_idx, pb_idx);
                if (grd_res == CORRECT) {
                    grd_sco = ans[pb_idx].score;
                }
                else if (grd_res == NO_FILE) {
                    grd_sco = 0;
                }
                else if (grd_res == MAKE_ERROR) {
                    grd_sco = ERROR_SCORE;
                }
                else if (grd_res == INCORRECT) {
                    grd_sco = 0;
                }
                else
                    grd_sco = 0;
                std[st_idx].score[pb_idx] = grd_sco;
                sprintf(grd_sco_s, "%.2f", grd_sco);
                write(sco_csv_fd, grd_sco_s, strlen(grd_sco_s));
                write(sco_csv_fd, ",", 1);
            }
            else if (strcmp(ans[pb_idx].type, ".c") == 0){
                grd_res = grading_program(st_idx, pb_idx);
                if (grd_res == CORRECT) {
                    grd_sco = ans[pb_idx].score;
                    grd_sco -= WARN_SCORE * warn_cnt;
                }
                else if (grd_res == NO_FILE) {
                    grd_sco = 0;
                }
                else if (grd_res == MAKE_ERROR) {
                    grd_sco = ERROR_SCORE;
                }
                else if (grd_res == INCORRECT) {
                    grd_sco = 0;
                }
                else
                    grd_sco = 0;
                std[st_idx].score[pb_idx] = grd_sco;
                sprintf(grd_sco_s, "%.2f", grd_sco);
                write(sco_csv_fd, grd_sco_s, strlen(grd_sco_s));
                write(sco_csv_fd, ",", 1);
            }
            sum += grd_sco;
        }
        sprintf(grd_sco_s, "%.2f", sum);
        write(sco_csv_fd, grd_sco_s, strlen(grd_sco_s));
        write(sco_csv_fd, "\n", 1);

        printf("%s is finished.. ", std[st_idx].name);
        if (flag_p) {
            all_score += sum;
            printf("score : %.2f", sum);
            sum = 0;
        }
        printf("\n");
    }
    if (flag_p) {
        all_score = all_score / st_count;
        printf("Total average : %.2f\n", all_score);
    }

    // -e option 이 없을 시 [err_dir] 삭제하기
    if (flag_e == 0) {
        if (access(err_dir_name, F_OK) == 0){ 
            char rm_dir[30] = "rm -rf ";
            strcat(rm_dir, err_dir_name);
            system(rm_dir);
        }
    }

    // pb_namelist 메모리 해제
    for (pb_idx = 0; pb_idx < pb_count; pb_idx++)
        free(pb_namelist[pb_idx]);
    free(pb_namelist);
    for (st_idx = 0; st_idx < st_count; st_idx++)
        free(st_list[st_idx]);
    free(st_list);

    gettimeofday(&end_t, NULL);
    ssu_runtime(&begin_t, &end_t);
    exit(0);
}

void print_usage()
{
    printf("Usage : ssu_score <STUDENTDIR> <TRUEDIR> [OPTION]\n");
    printf("Option :\n");
    printf("  -e <DIRNAME>     print error on 'DIRNAME/ID/qname_error.txt' file\n");
    //printf("  -t <QNAMEs>      compile QNAME.C with -lpthread option\n");
    printf("  -h               print usage\n");
    printf("  -p               print student's score and total average\n");
    //printf("  -c <IDS>         print ID's score\n");
    exit(0);
}

int filter(const struct dirent *dirent)
{   
    int is_pb_name;
    is_pb_name = atoi(dirent->d_name);
    if (is_pb_name == 0)
        return 0;
    else
        return 1;
}

int filter_2(const struct dirent *dirent)
{   
    char *fil = strchr(dirent->d_name, '.');
    if ((strcmp(fil , ".txt") != 0) && (strcmp(fil, ".c") != 0))
        return 0;
    else
        return 1;
}

void erase_space(char buf[])
{
    int index = 0;

    for (int i = 0; buf[i] != 0; i++)
        if (buf[i] != ' '){
            buf[index] = buf[i];
            index++;
        }
    buf[index] = 0;
}

int stricmp(char *cmp1, char *cmp2)
{
    for (int i = 0; (*cmp1 != 0 || *cmp2 != 0); i++) {
        if (*cmp1 >= 65 && *cmp1 <= 90)
            cmp1 += 32;
        if (*cmp2 >= 65 && *cmp2 <= 90)
            cmp2 += 32;
        if (*cmp1 > *cmp2)
            return 1;
        else if (*cmp1 < *cmp2)
            return -1;
        else{
            cmp1++;
            cmp2++;
        }
    }
    return 0;
}

int is_identifier(char buf)
{
    if (((buf >= 65) && (buf <= 90)) || ((buf >= 97) && (buf <= 122)) || (buf == 95) || ((buf >= 48) && (buf <= 57))) { // 알파벳, 숫자 또는 _(언더바) 일 경우
        return 1;
    }
    else{
        return 0;
    }
}

int grading_blank(int st_idx, int pb_dix)
{
    char st_path[30] = "";
    char pb_path[30] = "";
    int pb_fd, pb_length, st_fd, st_length;
    char pb_buf[1] = "";
    char st_buf[1] = "";
    char blank[1] = " ";
    char st_iden[100][50];
    int st_grp[100];
    char pb_iden[100][50];
    int iden_flag = 0;
    int head_flag = 0;;
    int st_cur_grp = 0;
    int st_iden_num = -1;
    int st_iden_buf_num = 0;
    int pb_iden_num = -1;
    int pb_iden_buf_num = 0;
    off_t offset;
    int cor_flag = 0;
    //st_path
    strcat(st_path, std_dir_name);
    strcat(st_path, "/");
    strcat(st_path, std[st_idx].name);
    strcat(st_path, "/");
    strcat(st_path, ans[pb_idx].num);
    strcat(st_path, ".txt");
    //pb_path
    strcat(pb_path, ans_dir_name);
    strcat(pb_path, "/");
    strcat(pb_path, ans[pb_idx].num);
    strcat(pb_path, "/");
    strcat(pb_path, ans[pb_idx].num);
    strcat(pb_path, ".txt");

    // 학생 파일 존재 여부 확인
    if (access(st_path, F_OK) < 0)
        return NO_FILE;
    if ((st_fd = open(st_path, O_RDONLY)) < 0) {
        fprintf(stderr, "open error for %s\n", st_path);
        return INCORRECT;
    }
    if ((pb_fd = open(pb_path, O_RDONLY)) < 0) {
        fprintf(stderr, "open error for %s\n", pb_path);
        exit(1);
    }
    /* 학생 답안 분석 */
    head_flag = 0;
    st_cur_grp = 0;
    while (1) {
        st_length = read(st_fd, &st_buf, 1);
        offset = lseek(st_fd, 0, SEEK_CUR); // offset 값을 계속 현재 오프셋 값으로 지정해주기
        if (st_length <= 0) {
            break;
        }
        else {
            if (is_identifier(st_buf[0]) == 1) { // 알파벳, 숫자 또는 _(언더바) 일 경우
                char tmp_buf[1] = "";
                pread(st_fd, &tmp_buf, 1, offset);
                if (is_identifier(tmp_buf[0]) == 0) { // 다음 버퍼가 다른 값일 때 
                    if (iden_flag == 0) { // 변수가 1바이트인 경우
                        st_iden_num++;
                        st_iden[st_iden_num][0] = st_buf[0];
                        st_iden[st_iden_num][1] = 0;
                        st_grp[st_iden_num] = st_cur_grp;
                    }
                    else {
                        st_iden_buf_num++;
                        st_iden[st_iden_num][st_iden_buf_num] = st_buf[0];
                        st_iden_buf_num++;
                        st_iden[st_iden_num][st_iden_buf_num] = 0;
                        iden_flag = 0;
                    }
                }
                else {
                    if (iden_flag == 0) { // 변수 시작일 때
                        iden_flag = 1;
                        st_iden_num++;
                        st_iden_buf_num = 0;
                        st_iden[st_iden_num][st_iden_buf_num] = st_buf[0];
                        st_grp[st_iden_num] = st_cur_grp;
                    }
                    else { //iden_flag == 1
                        st_iden_buf_num++;
                        st_iden[st_iden_num][st_iden_buf_num] = st_buf[0];
                    }
                }
            }
            else if ((st_buf[0] == ' ') || (st_buf[0] == '\n')) { // 공백
                iden_flag = 0;
            }
            else if (st_buf[0] == '(') { // 여는 괄호: 뒤에 부터 그룹 레벨 + 1
                st_iden_num++;
                strcpy(st_iden[st_iden_num], "(");
                st_grp[st_iden_num] = st_cur_grp;
                st_cur_grp++;
            }
            else if (st_buf[0] == ')') { // 닫는 괄호: 자신 부터 그룹 레벨 - 1
                st_iden_num++;
                strcpy(st_iden[st_iden_num], ")");
                st_cur_grp--;
                st_grp[st_iden_num] = st_cur_grp;
            }
            else if (st_buf[0] == ',') { //  자신 그룹 레벨만 + 1
                st_iden_num++;
                strcpy(st_iden[st_iden_num], ",");
                st_grp[st_iden_num] = st_cur_grp + 1;
            }
            else if (st_buf[0] == '*') { // 포인터 or 곱셈
                st_iden_num++;
                strcpy(st_iden[st_iden_num], "*");
                st_grp[st_iden_num] = st_cur_grp;
            }
            else if (st_buf[0] == ';') { // ';' 는 다른 변수로 간주
                st_iden_num++;
                strcpy(st_iden[st_iden_num],";");
                st_grp[st_iden_num] = st_cur_grp;
            }
            else if (st_buf[0] == '#') { // 헤더파일 문제
                head_flag = 1;
                st_iden_num++;
                strcpy(st_iden[st_iden_num], "#");
                st_grp[st_iden_num] = st_cur_grp;
            }
            else if (st_buf[0] == '"') { //  " ~a~a~a~a~a~ " 는 통째로 변수 하나
                st_iden_num++;
                st_iden_buf_num = 0;
                st_iden[st_iden_num][st_iden_buf_num] = st_buf[0];
                while (1) {
                    st_length = read(st_fd, &st_buf, 1);
                    if (st_length <= 0) // "가 안나왔는데 끝나면 틀린 답
                        return INCORRECT;
                    else if (st_buf[0] == '"') { // "가 나오면 닫고 종료
                        st_iden_buf_num++;
                        st_iden[st_iden_num][st_iden_buf_num] = st_buf[0];
                        st_iden_buf_num++;
                        st_iden[st_iden_num][st_iden_buf_num] = 0;
                        break;
                    }
                    else {
                        st_iden_buf_num++;
                        st_iden[st_iden_num][st_iden_buf_num] = st_buf[0];
                    }
                }
                st_grp[st_iden_num] = st_cur_grp;
            }
            else if (st_buf[0] == '<') {
                if (head_flag == 1) { // 헤더파일 문제일 경우
                    st_iden_num++;
                    st_iden_buf_num = 0;
                    st_iden[st_iden_num][st_iden_buf_num] = st_buf[0];
                    while (1) {
                        st_length = read(st_fd, &st_buf, 1);
                        if (st_length <= 0) //  > 가 안나왔는데 끝나면 틀린 답
                            return INCORRECT;
                        else if (st_buf[0] == '>') {
                            st_iden_buf_num++;
                            st_iden[st_iden_num][st_iden_buf_num] = st_buf[0];
                            st_iden_buf_num++;
                            st_iden[st_iden_num][st_iden_buf_num] = 0;
                            break;
                        }
                        else {
                            st_iden_buf_num++;
                            st_iden[st_iden_num][st_iden_buf_num] = st_buf[0];
                        }
                    }
                    st_grp[st_iden_num] = st_cur_grp;
                }
                else { // 헤더파일이 아니라 크기 부호로 간주
                    char tmp_buf[1] = "";
                    pread(st_fd, &tmp_buf, 1, offset);
                    if (tmp_buf[0] == '=') {
                        lseek(st_fd, 1, SEEK_CUR);
                        st_iden_num++;
                        strcpy(st_iden[st_iden_num], "<=");
                    }
                    else {
                        st_iden_num++;
                        strcpy(st_iden[st_iden_num], "<");
                    }
                    st_grp[st_iden_num] = -2;
                }
            }
            else if (st_buf[0] == '>') {
                if (head_flag == 1) { // 헤더파일 문제일 경우
                    return INCORRECT; // >가 먼저  나오면 틀린 답
                }
                else { // 크기 부호로 간주
                    char tmp_buf[1] = "";
                    pread(st_fd, &tmp_buf, 1, offset);
                    if (tmp_buf[0] == '=') {
                        lseek(st_fd, 1, SEEK_CUR);
                        st_iden_num++;
                        strcpy(st_iden[st_iden_num], ">=");
                    }
                    else {
                        st_iden_num++;
                        strcpy(st_iden[st_iden_num], ">");
                    }
                    st_grp[st_iden_num] = -2;
                }
            }
            else if (st_buf[0] == '=') { // '=' 1.대입 2.비교
                char tmp_buf[1] = "";
                pread(st_fd, &tmp_buf, 1, offset);
                if (tmp_buf[0] == '=') { // '==' 비교 연산자
                    lseek(st_fd, 1, SEEK_CUR);
                    st_iden_num++;
                    strcpy(st_iden[st_iden_num], "==");
                    st_grp[st_iden_num] = -1;
                }
                else { // '=' 대입 연산자
                    st_iden_num++;
                    strcpy(st_iden[st_iden_num], "=");
                    st_grp[st_iden_num] = st_cur_grp;
                }
            }
            else if (st_buf[0] == '|') {
                char tmp_buf[1] = "";
                pread(st_fd, &tmp_buf, 1, offset);
                if (tmp_buf[0] == '|') {
                    lseek(st_fd, 1, SEEK_CUR);
                    st_iden_num++;
                    strcpy(st_iden[st_iden_num], "||");
                    st_grp[st_iden_num] = -1;
                }
                else if (tmp_buf[0] == '=') {
                    lseek(st_fd, 1, SEEK_CUR);
                    st_iden_num++;
                    strcpy(st_iden[st_iden_num], "|=");
                    st_grp[st_iden_num] = st_cur_grp;
                }
                else {
                    st_iden_num++;
                    strcpy(st_iden[st_iden_num], "|");
                    st_grp[st_iden_num] = -1;
                }
            }
            else if (st_buf[0] == '&') {
                char tmp_buf[1] = "";
                pread(st_fd, &tmp_buf, 1, offset);
                if (tmp_buf[0] == '&') {
                    lseek(st_fd, 1, SEEK_CUR);
                    st_iden_num++;
                    strcpy(st_iden[st_iden_num], "&&");
                    st_grp[st_iden_num] = -1;
                }
                else if (tmp_buf[0] == '=') {
                    lseek(st_fd, 1, SEEK_CUR);
                    st_iden_num++;
                    strcpy(st_iden[st_iden_num], "&=");
                    st_grp[st_iden_num] = st_cur_grp;
                }
                else {
                    if ((is_identifier(st_iden[st_iden_num][0]) != 1) || (st_iden_num == -1) || (st_iden[st_iden_num][0] == '(')) {  // 앞에 피연산자가 아니거나, 문장 처음이거나, 괄호가 있다면  주소연산자로 간주
                        iden_flag = 1;
                        st_iden_num++;
                        st_iden_buf_num = 0;
                        st_iden[st_iden_num][st_iden_buf_num] = '&';
                        st_grp[st_iden_num] = st_cur_grp;
                    }
                    else {  // 모든 경우가 아니라면 and
                        st_iden_num++;
                        strcpy(st_iden[st_iden_num], "&");
                        st_grp[st_iden_num] = -1;
                    }
                }
            }
            else if (st_buf[0] == '!') {
                char tmp_buf[1] = "";
                pread(st_fd, &tmp_buf, 1, offset);
                if (tmp_buf[0] == '=') {
                    lseek(st_fd, 1, SEEK_CUR);
                    st_iden_num++;
                    strcpy(st_iden[st_iden_num], "!=");
                    st_grp[st_iden_num] = -1;
                }
                else {
                    st_iden_num++;
                    strcpy(st_iden[st_iden_num], "!");
                    st_grp[st_iden_num] = st_cur_grp;
                }
            }
            else if (st_buf[0] == '-') {
                char tmp_buf[1] = "";
                pread(st_fd, &tmp_buf, 1, offset);
                if (tmp_buf[0] == '>') {
                    lseek(st_fd, 1, SEEK_CUR);
                    st_iden_num++;
                    strcpy(st_iden[st_iden_num], "->");
                    st_grp[st_iden_num] = st_cur_grp;
                }
                else {
                    st_iden_num++;
                    strcpy(st_iden[st_iden_num], "-");
                    st_grp[st_iden_num] = st_cur_grp;
                }
            }
            else {
                st_iden_num++;
                st_iden_buf_num = 0;
                st_iden[st_iden_num][st_iden_buf_num] = st_buf[0];
                st_iden_buf_num++;
                st_iden[st_iden_num][st_iden_buf_num] = 0;
                st_grp[st_iden_num] = st_cur_grp;
            }   
        }
    }
    if (st_iden_num == -1)  // 아무 내용이 없다면
        return INCORRECT;

    int is_same = 0;
    int is_big = 0;
    int isame[5];
    int ibig[5];
    int cmp_cnt = 1;
    for (int i = 0; i <= st_iden_num; i++) {
        if (st_grp[i] == -1) {
            isame[is_same++] = i;
        }
        else if (st_grp[i] == -2) {
            ibig[is_big++] = i;
        }
    }

    /* 정답 파일 분석 */ //65~90 대문자, 97~122 소문자, 95 _    
    offset = 0;
    while (1) {   // 공백을 걸러주고 답안지로 인도해줌, 파일이 끝났으면 나가게 해줌
        iden_flag = 0;
        head_flag = 0;
        pb_iden_num = -1;
        pb_iden_buf_num = 0;
        pb_length = read(pb_fd, &pb_buf, 1);
        offset = lseek(pb_fd, 0, SEEK_CUR);
        if (pb_length <= 0) {
            break;
        }
        else if ((pb_buf[0] == ' ') || (pb_buf[0] == '\n')) {
        }
        else {
            lseek(pb_fd, -1, SEEK_CUR);
            while (1) {
                pb_length = read(pb_fd, &pb_buf, 1);
                offset = lseek(pb_fd, 0, SEEK_CUR); // offset 값을 계속 현재 오프셋 값으로 지정해주기
                if (pb_buf[0] == ':') {
                    break;
                }
                else if (pb_length <= 0) {
                    break;
                }
                else {
                    if (is_identifier(pb_buf[0]) == 1) { // 알파벳, 숫자 또는 _(언더바) 일 경우
                        char tmp_buf[1] = "";
                        pread(pb_fd, &tmp_buf, 1, offset);
                        if (is_identifier(tmp_buf[0]) == 0) { // 다음 버퍼가 다른 값일 때 
                            if (iden_flag == 0) { // 변수가 1바이트인 경우
                                pb_iden_num++;
                                pb_iden[pb_iden_num][0] = pb_buf[0];
                                pb_iden[pb_iden_num][1] = 0;
                            }
                            else {
                                pb_iden_buf_num++;
                                pb_iden[pb_iden_num][pb_iden_buf_num] = pb_buf[0];
                                pb_iden_buf_num++;
                                pb_iden[pb_iden_num][pb_iden_buf_num] = 0;
                                iden_flag = 0;
                            }
                        }
                        else {
                            if (iden_flag == 0) { // 변수 시작일 때
                                iden_flag = 1;
                                pb_iden_num++;
                                pb_iden_buf_num = 0;
                                pb_iden[pb_iden_num][pb_iden_buf_num] = pb_buf[0];
                            }
                            else { //iden_flag == 1
                                pb_iden_buf_num++;
                                pb_iden[pb_iden_num][pb_iden_buf_num] = pb_buf[0];
                            }
                        }
                    }
                    else if ((pb_buf[0] == ' ') || (pb_buf[0] == '\n')) { // 공백
                        iden_flag = 0;
                    }
                    else if (pb_buf[0] == '(') { // 여는 괄호: 뒤에 부터 그룹 레벨 + 1
                        pb_iden_num++;
                        strcpy(pb_iden[pb_iden_num], "(");
                    }
                    else if (pb_buf[0] == ')') { // 닫는 괄호: 자신 부터 그룹 레벨 - 1
                        pb_iden_num++;
                        strcpy(pb_iden[pb_iden_num], ")");
                    }
                    else if (pb_buf[0] == ',') { //  자신 그룹 레벨만 + 1
                        pb_iden_num++;
                        strcpy(pb_iden[pb_iden_num], ",");
                    }
                    else if (pb_buf[0] == '*') { // 포인터 or 곱셈
                        pb_iden_num++;
                        strcpy(pb_iden[pb_iden_num], "*");
                    }
                    else if (pb_buf[0] == ';') { // ';' 는 다른 변수로 간주
                        pb_iden_num++;
                        strcpy(pb_iden[pb_iden_num],";");
                    }
                    else if (pb_buf[0] == '#') { // 헤더파일 문제
                        head_flag = 1;
                        pb_iden_num++;
                        strcpy(pb_iden[pb_iden_num], "#");
                    }
                    else if (pb_buf[0] == '"') { //  " ~a~a~a~a~a~ " 는 통째로 변수 하나
                        pb_iden_num++;
                        pb_iden_buf_num = 0;
                        pb_iden[pb_iden_num][pb_iden_buf_num] = pb_buf[0];
                        while (1) {
                            pb_length = read(pb_fd, &pb_buf, 1);
                            if (pb_length <= 0) // "가 안나왔는데 끝나면 틀린 답
                                return INCORRECT;
                            else if (pb_buf[0] == '"') { // "가 나오면 닫고 종료
                                pb_iden_buf_num++;
                                pb_iden[pb_iden_num][pb_iden_buf_num] = pb_buf[0];
                                pb_iden_buf_num++;
                                pb_iden[pb_iden_num][pb_iden_buf_num] = 0;
                                break;
                            }
                            else {
                                pb_iden_buf_num++;
                                pb_iden[pb_iden_num][pb_iden_buf_num] = pb_buf[0];
                            }
                        }
                    }
                    else if (pb_buf[0] == '<') {
                        if (head_flag == 1) { // 헤더파일 문제일 경우
                            pb_iden_num++;
                            pb_iden_buf_num = 0;
                            pb_iden[pb_iden_num][pb_iden_buf_num] = pb_buf[0];
                            while (1) {
                                pb_length = read(pb_fd, &pb_buf, 1);
                                if (pb_length <= 0) //  > 가 안나왔는데 끝나면 틀린 답
                                    return INCORRECT;
                                else if (pb_buf[0] == '>') {
                                    pb_iden_buf_num++;
                                    pb_iden[pb_iden_num][pb_iden_buf_num] = pb_buf[0];
                                    pb_iden_buf_num++;
                                    pb_iden[pb_iden_num][pb_iden_buf_num] = 0;
                                    break;
                                }
                                else {
                                    pb_iden_buf_num++;
                                    pb_iden[pb_iden_num][pb_iden_buf_num] = pb_buf[0];
                                }
                            }
                        }
                        else { // 헤더파일이 아니라 크기 부호로 간주
                            char tmp_buf[1] = "";
                            pread(pb_fd, &tmp_buf, 1, offset);
                            if (tmp_buf[0] == '=') {
                                lseek(pb_fd, 1, SEEK_CUR);
                                pb_iden_num++;
                                strcpy(pb_iden[pb_iden_num], "<=");
                            }
                            else {
                                pb_iden_num++;
                                strcpy(pb_iden[pb_iden_num], "<");
                            }
                        }
                    }
                    else if (pb_buf[0] == '>') {
                        if (head_flag == 1) { // 헤더파일 문제일 경우
                            return INCORRECT; // >가 먼저  나오면 틀린 답
                        }
                        else { // 크기 부호로 간주
                            char tmp_buf[1] = "";
                            pread(pb_fd, &tmp_buf, 1, offset);
                            if (tmp_buf[0] == '=') {
                                lseek(pb_fd, 1, SEEK_CUR);
                                pb_iden_num++;
                                strcpy(pb_iden[pb_iden_num], ">=");
                            }
                            else {
                                pb_iden_num++;
                                strcpy(pb_iden[pb_iden_num], ">");
                            }
                        }
                    }
                    else if (pb_buf[0] == '=') { // '=' 1.대입 2.비교
                        char tmp_buf[1] = "";
                        pread(pb_fd, &tmp_buf, 1, offset);
                        if (tmp_buf[0] == '=') { // '==' 비교 연산자
                            lseek(pb_fd, 1, SEEK_CUR);
                            pb_iden_num++;
                            strcpy(pb_iden[pb_iden_num], "==");
                        }
                        else { // '=' 대입 연산자
                            pb_iden_num++;
                            strcpy(pb_iden[pb_iden_num], "=");
                        }
                    }
                    else if (pb_buf[0] == '|') {
                        char tmp_buf[1] = "";
                        pread(pb_fd, &tmp_buf, 1, offset);
                        if (tmp_buf[0] == '|') {
                            lseek(pb_fd, 1, SEEK_CUR);
                            pb_iden_num++;
                            strcpy(pb_iden[pb_iden_num], "||");
                        }
                        else if (tmp_buf[0] == '=') {
                            lseek(pb_fd, 1, SEEK_CUR);
                            pb_iden_num++;
                            strcpy(pb_iden[pb_iden_num], "|=");
                        }
                        else {
                            pb_iden_num++;
                            strcpy(pb_iden[pb_iden_num], "|");
                        }
                    }
                    else if (pb_buf[0] == '&') {
                        char tmp_buf[1] = "";
                        pread(pb_fd, &tmp_buf, 1, offset);
                        if (tmp_buf[0] == '&') {
                            lseek(pb_fd, 1, SEEK_CUR);
                            pb_iden_num++;
                            strcpy(pb_iden[pb_iden_num], "&&");
                        }
                        else if (tmp_buf[0] == '=') {
                            lseek(pb_fd, 1, SEEK_CUR);
                            pb_iden_num++;
                            strcpy(pb_iden[pb_iden_num], "&=");
                        }
                        else {
                            if ((is_identifier(pb_iden[pb_iden_num][0]) != 1) || (pb_iden_num == -1) || (pb_iden[pb_iden_num][0] == '(')) {  // 앞에 피연산자가 아니거나, 문장 처음이거나, 괄호가 있다면  주소연산자로 간주
                                iden_flag = 1;
                                pb_iden_num++;
                                pb_iden_buf_num = 0;
                                pb_iden[pb_iden_num][pb_iden_buf_num] = '&';
                            }
                            else {  // 모든 경우가 아니라면 and
                                pb_iden_num++;
                                strcpy(pb_iden[pb_iden_num], "&");
                            }
                        }
                    }
                    else if (pb_buf[0] == '!') {
                        char tmp_buf[1] = "";
                        pread(pb_fd, &tmp_buf, 1, offset);
                        if (tmp_buf[0] == '=') {
                            lseek(pb_fd, 1, SEEK_CUR);
                            pb_iden_num++;
                            strcpy(pb_iden[pb_iden_num], "!=");
                        }
                        else {
                            pb_iden_num++;
                            strcpy(pb_iden[pb_iden_num], "!");
                        }
                    }
                    else if (pb_buf[0] == '-') {
                        char tmp_buf[1] = "";
                        pread(pb_fd, &tmp_buf, 1, offset);
                        if (tmp_buf[0] == '>') {
                            lseek(pb_fd, 1, SEEK_CUR);
                            pb_iden_num++;
                            strcpy(pb_iden[pb_iden_num], "->");
                        }
                        else {
                            pb_iden_num++;
                            strcpy(pb_iden[pb_iden_num], "-");
                        }
                    }
                    else {
                        pb_iden_num++;
                        pb_iden_buf_num = 0;
                        pb_iden[pb_iden_num][pb_iden_buf_num] = pb_buf[0];
                        pb_iden_buf_num++;
                        pb_iden[pb_iden_num][pb_iden_buf_num] = 0;
                    }   
                }
            }
            /* 읽은 학생 답안이랑 정답 답안이 같은지 비교 */
            if (st_iden_num == pb_iden_num) { // 변수 갯수가 같은 경우
                cor_flag = 1;
                for (int i = 0; i <= pb_iden_num; i++) {
                    if (strcmp(st_iden[i], pb_iden[i]) != 0) {
                        cor_flag = 0;
                        break;
                    }
                }
                if (cor_flag == 1)
                    return CORRECT;
            }
        }
    }
    /*
    // 여러번 비교 해야하는지 알아보기
    if ((is_same != 0) && (is_big == 0)) {    // 비교 연산자만 있는 경우
    if (st_iden_num == pb_iden_num) {
    for (int i = 1; i <= is_same; i++)  // 생기는 경우의 수는 2의 (비교연산자 수) 승
    cmp_cnt = cmp_cnt * 2;
    for (int i = 0; i < is_same; i++) {
    if ((strcmp(st_iden[isame[i] - 1], "(") == 0) || (strcmp(st_iden[isame[i] + 1], ")") == 0)) {
    cor_flag = 1;
    for (int j = 0; j <= pb_iden_num; j++) {
    if (j < isame[i]) {
    j = j + (pb_iden_num - isame[i]
    }
    if (strcmp(st_iden[], pb_iden[i]) != 0) {
    cor_flag = 0;
    break;
    }
    }
    if (cor_flag = 1)
    return CORRECT;
    }
    }
    }

    }
    else if ((is_same == 0) && (is_big != 0)) {  // 크기 연산자만 있는 경우
    for(int i = 0; i <= st_iden_num; i++) {
    if ((strcmp(st_iden[i], "(") == 0) || (strcmp(st_iden[i], ")") == 0)) {
    for (int j = i; j <= st_iden_num - 1; j++) {
    strcpy(st_iden[j], st_iden[j+1]);
    }
    st_iden_num--;
    i--;
    }
    }
    for (int i = 0; i < is_big; i++) {
    for (int i = 0; i <= pb_iden_num; i++) {
    if (strcmp(st_iden[i], pb_iden[i]) != 0) {
    cor_flag = 0;
    break;
    }
    if (cor_flag == 1)
    return CORRECT;
    }
    cmp_cnt = cmp_cnt * 2;
    for (int i = 1; i <= cmp_cnt; i++) {
    for (int j = 0; j < is_big; j++) {

    }
    }
    }
    else if ((is_same != 0) && (is_big != 0)) {  // 비교,크기 연산자 모두 있는 경우

    }   
     */

close(st_fd);
close(pb_fd);

return INCORRECT;
}

int grading_program(int st_idx, int pb_idx)
{
    /* 학생 프로그램 실행 */
    char line[40] = "";
    char error_line[40] = "";
    char std_fname[15] = ""; //문제번호.c
    char pb_stdout[30] = "";
    char st_stdout[30] = "";
    int err_fd, st_fd, tmp_fd, pb_fd, err_size, sys;
    char st_buf[1] = " ";
    char pb_buf[1] = " ";
    char blank[1] = " ";
    int st_length = 0;
    int pb_length = 0;

    // warning 수  초기화
    warn_cnt = 0;
    // [err_dir]/학번 directory 생성
    chdir(err_dir_name);
    if (access(std[st_idx].name, F_OK) < 0) {
        mkdir(std[st_idx].name, 0777); 
    }
    chdir("../");

    /* 학생 답안 확인하기 */
    if (chdir(std_dir_name) < 0) {
        fprintf(stderr, "chdir error\n");
        exit(1);
    }
    if (chdir(std[st_idx].name) < 0) {
        fprintf(stderr, "chdir error\n");
        exit(1);
    }
    //  문제 번호.c 존재 여부 확인
    strcat(std_fname, ans[pb_idx].num);
    strcat(std_fname, ".c");
    if (access(std_fname, F_OK) < 0) {
        chdir("../../");
        return NO_FILE;
    }
    //  [err_dir]/학번/문제번호_error.txt 생성
    strcat(error_line, "../../");
    strcat(error_line, err_dir_name);
    strcat(error_line, "/");
    strcat(error_line, std[st_idx].name);
    strcat(error_line, "/");
    strcat(error_line, ans[pb_idx].num);
    strcat(error_line, "_error.txt");
    if ((err_fd = open(error_line, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
        fprintf(stderr, "open error for %s\n", st_stdout);
        exit(1);
    }
    tmp_fd = dup(2);
    dup2(err_fd, 2);
    //  문제번호.stdexe 생성
    strcat(line, "gcc ");
    strcat(line, ans[pb_idx].num);
    strcat(line, ".c -o ");
    strcat(line, ans[pb_idx].num);
    strcat(line, ".stdexe");
    if (ans[pb_idx].flag_t == 1)
        strcat(line, " -lpthread");
    if ((sys = system(line)) == 256) {
        dup2(tmp_fd, 2);
        close(err_fd);
        chdir("../../");
        return MAKE_ERROR;     
    }
    else {
        if (lseek(err_fd, 0, SEEK_END) != 0) {
            dup2(tmp_fd, 2);
            close(err_fd);
            warn_cnt++;
        }
        remove(error_line);
        //문제번호.stdout 생성
        strcat(st_stdout, ans[pb_idx].num);
        strcat(st_stdout, ".stdout");
        if ((st_fd = open(st_stdout, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0) {
            fprintf(stderr, "open error for %s\n", st_stdout);
            exit(1);
        }
        tmp_fd = dup(1);
        dup2(st_fd, 1);
        strcpy(line, "./");
        strcat(line, ans[pb_idx].num);
        strcat(line, ".stdexe");
        system(line);
        dup2(tmp_fd, 1);
        close(st_fd);
    }
    if (chdir("../../") < 0) {
        fprintf(stderr, "chdir error\n");
        exit(1);
    }

    /* 학생 답안 체크 */    
    //pb_stdout path
    strcat(pb_stdout, ans_dir_name);
    strcat(pb_stdout, "/");
    strcat(pb_stdout, ans[pb_idx].num);
    strcat(pb_stdout, "/");
    strcat(pb_stdout, ans[pb_idx].num);
    strcat(pb_stdout, ".stdout");
    //st_stdout path
    strcpy(st_stdout, "");
    strcat(st_stdout, std_dir_name);
    strcat(st_stdout, "/");
    strcat(st_stdout, std[st_idx].name);
    strcat(st_stdout, "/");
    strcat(st_stdout, ans[pb_idx].num);
    strcat(st_stdout, ".stdout");
    //문제번호.stdout 비교
    if ((pb_fd = open(pb_stdout, O_RDONLY)) < 0) {
        fprintf(stderr, "open error for %s\n", pb_stdout);
        exit(1);
    }
    if ((st_fd = open(st_stdout, O_RDONLY)) < 0) {
        fprintf(stderr, "open error foe %s\n", st_stdout);
        return INCORRECT;
    }
    lseek(pb_fd, 0, SEEK_SET);
    lseek(st_fd, 0, SEEK_SET);
    while (1) {
        while(1) {
            pb_length = read(pb_fd, &pb_buf, 1);
            if (strcmp(pb_buf, blank) == 0) {
                pb_length = read(pb_fd, &pb_buf, 1);
            }
            else {
                break;
            }
        }               
        while(1) {
            st_length = read(st_fd, &st_buf, 1);
            if (strcmp(st_buf, blank) == 0)
                st_length = read(st_fd, &st_buf, 1);
            else
                break;
        }
        if ((pb_length <= 0) && (st_length > 0))
            return INCORRECT;
        else if ((pb_length > 0) && (st_length <= 0))
            return INCORRECT;
        else if ((pb_length <= 0) && (st_length <= 0))
            return CORRECT;
        //대문자를 소문자로 변환
        if (pb_buf[0] >= 65 && pb_buf[0] <= 90)
            pb_buf[0] += 32;
        if (st_buf[0] >= 65 && st_buf[0] <= 90)
            st_buf[0] += 32;
        //비교
        if (pb_buf[0] == st_buf[0]){
            continue;
        }
        else {
            return INCORRECT;
        }
    }
    close(pb_fd);
    close(st_fd);

    return CORRECT;
}
