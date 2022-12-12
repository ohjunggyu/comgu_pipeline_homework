//
//  main.cpp
//  MIPS Assembler
//
//  Created by Young Seok Kim
//  Copyright © 2015 TonyKim. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <stdlib.h>
using namespace std;

// data structures
struct label {
    char name[100];
    int location;
};

// Global variables
label label_list[100];
int labelindex = 0;

int instructions[1000];
int instr_index = 0;

int datasection[1000];
int datasectionindex = 0;

int Regsection[32];

// functions
void scanLabels(char *filename);
int countDataSection(char *filename);

void assembleLine(char *line);

void makeR_type(int op, int rs, int rt, int rd, int shamt, int funct);
void makeI_type(int op, int rs, int rt, int addr);
void makeJ_type(int op, int addr);

int regToInt(char *reg);
int labelToIntAddr(char *label);
int immToInt(char *immediate);
char *int2bin(int a, char *buffer, int buf_size);

int ALU_operate(int op, int r1, int r2);
int ALU_control(int op, int funct);

void Forwarding_Unit(int Rs, int Rt);
void Hazard_detection(IF_ID IF_, ID_EX ID_, EX_MEM EX_);


int INIT_PC = 0x00400024;
// Hazard
int ForwardA, ForwardB;
int PCWrite, IF_IDWrite, ControlMUX;
int IF_Flush, PC_;
// IF/ID
struct IF_ID{
    // Flush가 1이면 0!!
    int PC;
    int instruction;
    IF_ID(){}
    IF_ID(int PC_, int instruction_){
        PC = PC_;
        instruction = instruction_;
    }
}IF;

// ID/EX
struct ID_EX{
    int Branch;
    int ALUSrc, ALUOp, RegDst;
    int MemWrite, MemRead;
    int RegWrite, MemtoReg;

    int PC;

    int Register1;
    int Register2;

    int inst;       // 15:0
    int Rs;         // 25:21
    int Rt;         // 20:16
    int Rd;         // 15:11

    int op,shamt,Func,Rs;
    ID_EX(){}
    ID_EX(IF_ID tmp){        
        PC = tmp.PC;

        inst = (tmp.instruction) & ((1<<16)-1);
        for(int i=16;i<32;i++) inst |= (inst & (1<<15)); // sign_extend
        
        Rs = (tmp.instruction>>21) & ((1<<5)-1);
        Rd = (tmp.instruction>>11) & ((1<<5)-1);
        Rt = (tmp.instruction>>16) & ((1<<5)-1);

        Func = tmp.instruction & ((1<<6) - 1);
        shamt = (tmp.instruction>>6) & ((1<<5)-1);
        op = (tmp.instruction>>26) & ((1<<6)-1);

        // Register1,2
        Register1 = Regsection[Rs]; //datasection[Rs];
        Register2 = Regsection[Rt]; //datasection[Rt];

        // ALUOp
        switch(op){
            // RegDst
            // ALUSrc
            // MemtoReg
            // RegWrite
            // MemRead
            // MemWrite
            // Branch
            // ALUOp
            case 0x23:  //lw
                RegDst = 0;
                ALUSrc = 1;
                MemtoReg = 1;
                RegWrite = 1;
                MemRead = 1;
                MemWrite = 0;
                Branch = 0;
                ALUOp = 0;
                break;
                
            case 0x2b:  //sw
                // RegDst
                ALUSrc = 1;
                // MemtoReg
                RegWrite = 0;
                MemRead = 0;
                MemWrite = 1;
                Branch = 0;
                ALUOp = 0;
                break;
                
            case 0x04:  //beq
                // RegDst
                ALUSrc = 0;
                // MemtoReg
                RegWrite = 0;
                MemRead = 0;
                MemWrite = 0;
                Branch = 1;
                ALUOp = 1;

                if(Register1 == Register2){
                    IF_Flush = 1;
                    PC_ = PC + (inst << 2);
                }
                break;
                
            case 0:     //R
                RegDst = 1;
                ALUSrc = 0;
                MemtoReg = 0;
                RegWrite = 1;
                MemRead = 0;
                MemWrite = 0;
                Branch = 0;
                ALUOp = 2;
                break;
            
            case 8:     // addi
                Func = 32;  // 100000
                RegDst = 1;
                ALUSrc = 0;
                MemtoReg = 0;
                RegWrite = 1;
                MemRead = 0;
                MemWrite = 0;
                Branch = 0;
                ALUOp = 2;
                break;
    
            case 10:    // slti
                Func = 42;  // 101010
                RegDst = 1;
                ALUSrc = 0;
                MemtoReg = 0;
                RegWrite = 1;
                MemRead = 0;
                MemWrite = 0;
                Branch = 0;
                ALUOp = 2;
                break;
    
            case 12:    // andi
                Func = 36;  // 100100
                RegDst = 1;
                ALUSrc = 0;
                MemtoReg = 0;
                RegWrite = 1;
                MemRead = 0;
                MemWrite = 0;
                Branch = 0;
                ALUOp = 2;
                break;
    
            case 13:    // ori
                Func = 37;  // 100101
                RegDst = 1;
                ALUSrc = 0;
                MemtoReg = 0;
                RegWrite = 1;
                MemRead = 0;
                MemWrite = 0;
                Branch = 0;
                ALUOp = 2;
                break;
            
            case 2: // J
                Func = 0;
                RegDst = 0;
                ALUSrc = 0;
                MemtoReg = 0;
                RegWrite = 0;
                MemRead = 0;
                MemWrite = 0;
                Branch = 1;
                
                PC_ = PC + (inst << 2);
                IF_Flush = 1;

                ALUOp = 2;
                break;

            // and/or/add/sub/slt/lw/sw/beq/j
            // 13 ori, 9 addiu, 12 andi
            
            // ALUSrc = (I format이면 1 아니면 0)
            // PCSrc = j 또는 slt일때 (Branch랑 Zero랑 And 연산??)
            // Branch = ??

            // MemRead
            // MemWrite
        }       
    }
}ID;

// EX/MEM
struct EX_MEM{
    int ALUSrc, RegDst, ALUOp;
    int RegWrite, MemtoReg, MemRead, MemWrite;

    int ALUResult;
    int Register2;
    int RegisterRd;

    EX_MEM(){}
    EX_MEM(ID_EX tmp){
        ALUSrc = tmp.ALUSrc;
        RegDst = tmp.RegDst;
        ALUOp = tmp.ALUOp;

        RegWrite = tmp.RegWrite;
        MemtoReg = tmp.MemtoReg;
        MemRead = tmp.MemRead;
        MemWrite = tmp.MemWrite;

        RegisterRd = tmp.RegDst ? tmp.Rd : tmp.Rt;

        // Forwarding!
        Forwarding_Unit(tmp.Rs, tmp.Rt);
        int r1,r2;

        r1 = (ForwardA) ? ForwardA : tmp.Register1;
        r2 = (ForwardB) ? ForwardB : ((tmp.ALUSrc) ? tmp.inst : tmp.Register2);
        
        // ALU!
        ALUResult = ALU_operate(ALU_control(tmp.op,tmp.Func),r1,r2);
        Register2 = tmp.Register2;
    }    
}EX;

// MEM/WB
struct MEM_WB{
    int RegWrite, MemtoReg, MemRead, MemWrite;

    int Register2;
    int ALUResult;
    int RegisterRd;

    int result;

    MEM_WB(){}
    MEM_WB(EX_MEM tmp){
        RegWrite = tmp.RegWrite;
        MemtoReg = tmp.MemtoReg;
        MemRead = tmp.MemRead;
        MemWrite = tmp.MemWrite;
        
        Register2 = tmp.Register2;
        ALUResult = tmp.ALUResult;
        RegisterRd = tmp.RegisterRd;

        if(MemWrite){
            datasection[ALUResult] = Register2;
        }
        if(RegWrite){
            Regsection[RegisterRd] = (MemtoReg) ? datasection[ALUResult] : ALUResult; // MemtoReg가 1이면 mem 0이면 ALURes
        }
    }
}MEM;

void Forwarding_Unit(int Rs, int Rt){
    int exdst, exwrite, memdst, memwrite;
    exdst = EX.RegisterRd;
    exwrite = EX.RegWrite;

    ForwardA = 0;
    if(Rs == memdst && memwrite) ForwardA = EX.ALUResult;
    if(Rs == exdst && exwrite) ForwardA = MEM.result;

    memdst = MEM.RegisterRd;
    memwrite = MEM.RegWrite;
    ForwardB = 0;
    if(Rt == memdst && memwrite) ForwardB = EX.ALUResult;
    if(Rt == exdst && exwrite) ForwardB = MEM.result;
}
/************************ MAIN ************************/

void Hazard_detection(IF_ID IF_, ID_EX ID_, EX_MEM EX_){
    if(EX_.MemRead && (ID_.Rs == ID.Rt || ID_.Rt == ID.Rt)){
        PCWrite = IF_IDWrite = ControlMUX = 0;
    }else{
        PCWrite = IF_IDWrite = ControlMUX = 1;
    }
}

int main(/*int argc, const char * argv[]*/) {
    
    
    /*if (argv[1] == NULL) {
        printf("Please type in input file name.\n");
        return 0;
    }*/
     
    char filename[20];
    //strcpy(filename, argv[1]);
    
    strcpy(filename, "example_1.s");
    
    int dataSectionSize = countDataSection(filename);
    scanLabels(filename);
    
    string line;
    ifstream inputfile(filename);
    if (inputfile.is_open())
    {
        while (getline(inputfile, line) )
        {
            if(strstr(line.c_str(),".text") != NULL){
                break;
            }
        }
        
        while (getline(inputfile, line) )
        {
            if (strstr(line.c_str(), ":") != NULL) {
                continue;
            } else {
                char codeline[100];
                strcpy(codeline, line.c_str());
                assembleLine(codeline);
            }
        }
        inputfile.close();
    }
    else cout << "Unable to open file\n";
    
    
    /* Writing section */
    /*
    ofstream outputfile("output.o", ios::out | ios::binary);
    outputfile.put(instr_index*4); // text section size
    outputfile.put(dataSectionSize*4); // data section size
    int i;
    for (i=0; i<instr_index; i++) {
        outputfile.put(instructions[i]);
    }
    for (i=0; i<datasectionindex; i++) {
        outputfile.put(datasection[i]);
    }
    outputfile.close();
    */
    //FILE f = *fopen("output.o", "w");
    freopen("output.o","w",stdout);
    char binary[40];
    int2bin(instr_index*4, binary, 32);
    printf("%s\n", binary);
    int2bin(dataSectionSize*4, binary, 32);
    printf("%s\n", binary);
    
    int i=0, PC = INIT_PC;
    while(1){

        // MEM/WB
        MEM_WB MEM_ = MEM_WB(EX);

        // EX/MEM
        EX_MEM EX_ = EX_MEM(ID);
        
        // ID/EX
        ID_EX ID_ = ID_EX(IF);

        // IF/ID
        IF_ID IF_ = {0,0};
        if(IF_Flush == 1){
            i = PC_/4;
            IF_Flush = 0;
        }
        else if(i<instr_index) 
            IF_ = {i*4, instructions[i++]};
        
        Hazard_detection(IF_, ID_, EX_);
        MEM = MEM_;
        EX = EX_;
        if(ControlMUX == 0) EX.MemRead = EX.MemWrite = EX.RegWrite = 0;
        if(IF_IDWrite) ID = ID_;
        if(PCWrite) IF = IF_;
        
    }
    for (i=0; i<instr_index; i++) {
        int2bin(instructions[i], binary, 32);
        printf("%s\n", binary);
    }
    for (i=0; i<datasectionindex; i++) {
        int2bin(datasection[i], binary, 32);
        printf("%s\n", binary);
    }
    //printf("\n");
    //fclose(&f);
    return 0;
}


void scanLabels(char *filename) {
    int datasizecnt = 0;
    int instruction_count = 0;
    
    string line;
    ifstream inputfile(filename);
    if (inputfile.is_open())
    {
        // finds the .data section and count the number of .word before .text section appears
        /*while (getline(inputfile, line))
        {
            if (strstr(line.c_str(),".data") != NULL){
                break;
            }
        }*/
        while (getline(inputfile, line)) {
            // Labels inside .data section.
            /*if (strstr(line.c_str(),".word") != NULL){
                label newLabel;
                char temp[100];
                strcpy(temp, line.c_str());
                strcpy(newLabel.name , strtok(temp,":"));
                newLabel.location = 0x10000000+(datasizecnt)*4;
                datasizecnt++;
                label_list[labelindex++]=newLabel;
            }*/
            if (strstr(line.c_str(),".text") != NULL){
                break;
            }
        }
        // Labels inside .text section (function labels)
        while (getline(inputfile, line)) {
            if (strstr(line.c_str(), ":") != NULL) {
                label newLabel;
                char temp[100];
                strcpy(temp, line.c_str());
                strcpy(newLabel.name , strtok(temp,":"));
                newLabel.location = 0x400000 + instruction_count*4;
                label_list[labelindex++]=newLabel;
            } else {
                char temp[100];
                strcpy(temp, line.c_str());
                
                char * one;
                char * two;
                char * three = NULL;
                char key[] = " ,\t";
                one = strtok(temp, key);
                if(one != NULL)
                    two = strtok(NULL,key);
                if(two != NULL)
                    three = strtok(NULL,key);
                
                if (strcmp(one, "la") == 0){
                    int location = labelToIntAddr(three);
                    if ((location & 65535) == 0) {          // if lower 16bit address is 0x0000
                        instruction_count++;                // lui instruction
                    } else {
                        instruction_count += 2;             // lui instruction + ori instruction
                    }
                } else {
                    instruction_count++;
                }
            }
        }
        
        
        inputfile.close();
    }
    else cout << "Unable to open file\n";
}


int countDataSection(char *filename) {
    int count = 0;
    
    string line;
    ifstream inputfile(filename);
    if (inputfile.is_open())
    {
        /*
        // finds the .data section and count the number of .word before .text section appears
        while (getline(inputfile, line))
        {
            if(strstr(line.c_str(),".data") != NULL){
                break;
            }
        }
        while (getline(inputfile, line)) {
            if(strstr(line.c_str(),".text") != NULL){
                break;
            }
            if(strstr(line.c_str(),".word") != NULL){
                count++;
                char * one;
                char * two;
                char * three;
                char temp[100];
                strcpy(temp, line.c_str());
                
                if (strstr(temp,":") != NULL) {
                    one = strtok(temp,":");
                    two = strtok(NULL, " \t");
                    three = strtok(NULL, " \t");
                    datasection[datasectionindex++] = immToInt(three);
                } else {
                    one = strtok(temp, " \t");
                    two = strtok(NULL, " \t");
                    datasection[datasectionindex++] = immToInt(two);
                }
           
            }
        }
        inputfile.close();
        */
    }
    else cout << "Unable to open file\n";
    return count;
}




/* Assembling Function */
void assembleLine(char *line){

    char key[] = " ,\t";
    char * one;
    char * two;
    char * three;
    char * four = NULL;
    
    one = strtok(line,key);
    
    if(one != NULL)
        two = strtok(NULL,key);
    if(two != NULL)
        three = strtok(NULL,key);
    if(three != NULL)
        four = strtok(NULL,key);
    
    
    if (strcmp(one, "addi") == 0) {
        // I-type
        // addi $t,$s,C
        // $t = $s + C (unsigned)
        // R[rt] = R[rs] + SignExtImm
        makeI_type(8, regToInt(three), regToInt(two), immToInt(four));
        
    } else if (strcmp(one, "addu") == 0) {
        // R-type
        // addu $d,$s,$t
        // $d = $s + $t
        // R[rd] = R[rs] + R[rt]
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 33);
        
    } else if (strcmp(one, "andi") == 0) {
        // I-type
        // andi $t,$s,C
        // $t = $s & C
        // R[rt] = R[rs] + SignExtImm
        makeI_type(12, regToInt(three), regToInt(two), immToInt(four));
        
    } else if (strcmp(one, "and") == 0) {
        // R-type
        // and $d,$s,$t
        // $d = $s & $t
        // R[rd] = R[rs] & R[rt]
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 36);
        
    } else if (strcmp(one, "beq") == 0) {
        // I-type
        // beq $s,$t,C
        // if ($s == $t) go to PC+4+4*C
        // if(R[rs]==R[rt]) PC=PC+4+BranchAddr
        int reladdr = labelToIntAddr(four) - (0x400000 + ((instr_index)*4)+4);
        makeI_type(4, regToInt(two), regToInt(three), (reladdr/4));          // relative address
        
    } else if (strcmp(one, "bne") == 0) {
        // I-type
        // bne $s,$t,C
        // if ($s != $t) go to PC+4+4*C
        // if(R[rs]!=R[rt]) PC=PC+4+BranchAddr
        int reladdr = labelToIntAddr(four) - (0x400000 + ((instr_index)*4)+4);
        makeI_type(5, regToInt(two), regToInt(three), (reladdr/4));          // relative address
        
    } else if (strcmp(one, "jal") == 0) {
        // J-type
        // jal C
        // $31 = PC + 4; PC = PC+4[31:28] . C*4
        // R[31]=PC+8;PC=JumpAddr
        makeJ_type(3, (labelToIntAddr(two)>>2));                             // absolute address
        
    } else if (strcmp(one, "jr") == 0) {
        // R-type
        // jr $s
        // goto address $s
        // PC=R[rs]
        makeR_type(0, regToInt(two), 0, 0, 0, 8);
        
    } else if (strcmp(one, "j") == 0) {
        // J-type
        // j C
        // PC = PC+4[31:28] . C*4
        // PC=JumpAddr
        makeJ_type(2, (labelToIntAddr(two)>>2));                             // absolute address
    }
    else if (strcmp(one, "lui") == 0) {
        // I-type
        // lui $t,C
        // $t = C << 16
        // R[rt] = {imm, 16’b0}
        makeI_type(15, 0, regToInt(two), immToInt(three));
        
    } else if (strcmp(one, "lw") == 0) {
        // I-type
        // lw $t,C($s)
        // $t = Memory[$s + C]
        // R[rt] = M[R[rs]+SignExtImm]
        char * offset = strtok(three,"()");
        char * rs = strtok(NULL,"()");
        makeI_type(35, regToInt(rs), regToInt(two), immToInt(offset));
        
    } else if (strcmp(one, "la") == 0) { // special
        // If the lower 16bit address is 0x0000, the ori instruction is useless.
        /*
         Case1) load address is 0x1000 0000 
         lui $2, 0x1000
         
         Case2) load address is 0x1000 0004 
         lui $2, 0x1000
         ori $2, $2, 0x0004
        */
        int location = labelToIntAddr(three);                                   // absolute label address
        if ((location & 65535) == 0) { // if lower 16bit address is 0x0000
            makeI_type(15, 0, regToInt(two), (location>>16));                   // lui instruction
        } else {
            makeI_type(15, 0, regToInt(two), (location>>16));                   // lui instruction
            makeI_type(13, regToInt(two), regToInt(two), (location & 65535));   // ori instruction
        }
        
    } else if (strcmp(one, "nor") == 0) {
        // R-type
        // nor $d,$s,$t
        // $d = ~ ($s | $t)
        // R[rd] = ~ (R[rs] | R[rt])
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 39);
        
    } else if (strcmp(one, "ori") == 0) {
        // I-type
        // ori $t,$s,C
        // $t = $s | C
        // R[rt] = R[rs] | ZeroExtImm
        makeI_type(13, regToInt(three), regToInt(two), immToInt(four));
        
    } else if (strcmp(one, "or") == 0) {
        // R-type
        // or $d,$s,$t
        // $d = $s | $t
        // R[rd] = R[rs] | R[rt]
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 37);
        
    }else if (strcmp(one, "slti") == 0) {
        // I-type
        // sltiu $t,$s,C
        // R[rt] = (R[rs] < SignExtImm) ? 1 :0
        makeI_type(10, regToInt(three), regToInt(two), immToInt(four));
        
    }else if (strcmp(one, "sltiu") == 0) {
        // I-type
        // sltiu $t,$s,C
        // R[rt] = (R[rs] < SignExtImm) ? 1 :0
        makeI_type(11, regToInt(three), regToInt(two), immToInt(four));
        
    } else if (strcmp(one, "sltu") == 0) {
        // R-type
        // sltu $d,$s,$t
        // R[rd] = (R[rs] < R[rt]) ? 1 : 0
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 43);
        
    } else if (strcmp(one, "sll") == 0) {
        // R-type
        // sll $d,$t,shamt
        // $d = $t << shamt
        // R[rd] = R[rt] << shamt
        makeR_type(0, 0, regToInt(three), regToInt(two), immToInt(four), 0);
        
    } else if (strcmp(one, "srl") == 0) {
        // R-type
        // srl $d,$t,shamt
        // $d = $t >> shamt
        // R[rd] = R[rt] >> shamt
        makeR_type(0, 0, regToInt(three), regToInt(two), immToInt(four), 2);
        
    } else if (strcmp(one, "sw") == 0) {
        // I-type
        // sw $t,C($s)
        // Memory[$s + C] = $t
        // M[R[rs]+SignExtImm] = R[rt]
        char * offset = strtok(three,"()");
        char * rs = strtok(NULL,"()");
        makeI_type(43, regToInt(rs), regToInt(two), immToInt(offset));
        
    } else if (strcmp(one, "sub") == 0) {
        // R-type
        // sub $d,$s,$t
        // $d = $s - $t
        // R[rd] = R[rs] - R[rt]
        makeR_type(0, regToInt(three), regToInt(four), regToInt(two), 0, 34);
    }
}



// MIPS Fields
/*
 
 < R-type >
  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 |    op(6bits)    |   rs(5bits)  |   rt(5bits)  |   rd(5bits)  | shamt(5bits) |   fucnt(6bits)  |
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 
 
 < I-type >
  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 |    op(6bits)    |   rs(5bits)  |   rt(5bits)  |         constant or address (16bits)          |
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 
 < J-type >
  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 |    op(6bits)    |                            address (30bits)                                 |
 +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 
*/


void makeR_type(int op, int rs, int rt, int rd, int shamt, int funct) {
    int ans = 0;
    ans = (op << 26);
    ans |= (rs << 21);
    ans |= (rt << 16);
    ans |= (rd << 11);
    ans |= (shamt << 6);
    ans |= funct;
    instructions[instr_index++] = ans;
}

void makeI_type(int op, int rs, int rt, int addr) {
    int ans = 0;
    ans = (op << 26);
    ans |= (rs << 21);
    ans |= (rt << 16);
    ans |= (addr & 65535);
    instructions[instr_index++] = ans;
}

void makeJ_type(int op, int addr) {
    int ans = 0;
    ans = (op << 26);
    ans |= addr;
    instructions[instr_index++] = ans;
}

int ALU_operate(int op, int r1, int r2){
    switch (op)
    {
        case 0: return r1 & r2;
        case 1: return r1 | r2;
        case 2: return r1 + r2;
        case 6: return r1 - r2;
        case 7: return r1 < r2; // ????
        case 12:return ~(r1 | r2);
    }
    return 0;
}

int ALU_control(int op, int funct){
    funct = 15 & funct;
    if(op == 0){
        return 2;   // 0010 AND
    }else if(op == 1){
        return 6;   // 0110 subtract
    }else if(op == 2){
        if(funct == 0) return 2;             // 0010 add
        else if(funct == 2) return 6;        // 0110 subtract
        else if(funct == 4) return 0;        // 0000 AND
        else if(funct == 5) return 1;        // 0001 OR
        else if(funct == 10) return 7;       // 0111 slt
    }
    return -1;
}



// Conversion functions

int regToInt(char *reg){
    //$ 빼고 int로 바꾸기.
    char * cut = strtok(reg,"$");
    return atoi(cut);
}

int labelToIntAddr(char *label) {
    int i;
    for (i=0; i<labelindex; i++) {
        if (strcmp(label_list[i].name, label) == 0) {
            return label_list[i].location;
        }
    }
    return -1;
}

int immToInt(char *immediate){
    // 2 cases - hexadecimal and decimal.
    if (strstr(immediate,"0x") != NULL) {
        return (int)strtol(immediate, NULL, 0);
    } else {
        return atoi(immediate);
    }
}

char *int2bin(int a, char *buffer, int buf_size) {
    buffer += (buf_size - 1);
    for (int i = 31; i >= 0; i--) {
        *buffer-- = (a & 1) + '0';
        a >>= 1;
    }
    return buffer;
}
