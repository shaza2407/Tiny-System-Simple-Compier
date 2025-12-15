#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
using namespace std;

// sequence of statements separated by ;
// no procedures - no declarations
// all variables are integers
// variables are declared simply by assigning values to them :=
// if-statement: if (boolean) then [else] end
// repeat-statement: repeat until (boolean)
// boolean only in if and repeat conditions < = and two mathematical expressions
// math expressions integers only, + - * / ^
// I/O read write
// Comments {}

////////////////////////////////////////////////////////////////////////////////////
// Strings /////////////////////////////////////////////////////////////////////////

bool Equals(const char* a, const char* b)
{
    return strcmp(a, b)==0;
}

bool StartsWith(const char* a, const char* b)
{
    int nb=strlen(b);
    return strncmp(a, b, nb)==0;
}

void Copy(char* a, const char* b, int n=0)
{
    if(n>0) {strncpy(a, b, n); a[n]=0;}
    else strcpy(a, b);
}

void AllocateAndCopy(char** a, const char* b)
{
    if(b==0) {*a=0; return;}
    int n=strlen(b);
    *a=new char[n+1];
    strcpy(*a, b);
}

////////////////////////////////////////////////////////////////////////////////////
// Input and Output ////////////////////////////////////////////////////////////////

#define MAX_LINE_LENGTH 10000

struct InFile
{
    FILE* file;
    int cur_line_num;

    char line_buf[MAX_LINE_LENGTH];
    int cur_ind, cur_line_size;

    InFile(const char* str) {file=0; if(str) file=fopen(str, "r"); cur_line_size=0; cur_ind=0; cur_line_num=0;}
    ~InFile(){if(file) fclose(file);}

    void SkipSpaces()
    {
        while(cur_ind<cur_line_size)
        {
            char ch=line_buf[cur_ind];
            if(ch!=' ' && ch!='\t' && ch!='\r' && ch!='\n') break;
            cur_ind++;
        }
    }

    bool SkipUpto(const char* str)
    {
        while(true)
        {
            SkipSpaces();
            while(cur_ind>=cur_line_size) {if(!GetNewLine()) return false; SkipSpaces();}

            if(StartsWith(&line_buf[cur_ind], str))
            {
                cur_ind+=strlen(str);
                return true;
            }
            cur_ind++;
        }
        return false;
    }

    bool GetNewLine()
    {
        cur_ind=0; line_buf[0]=0;
        if(!fgets(line_buf, MAX_LINE_LENGTH, file)) return false;
        cur_line_size=strlen(line_buf);
        if(cur_line_size==0) return false; // End of file
        cur_line_num++;
        return true;
    }

    char* GetNextTokenStr()
    {
        SkipSpaces();
        while(cur_ind>=cur_line_size) {if(!GetNewLine()) return 0; SkipSpaces();}
        return &line_buf[cur_ind];
    }

    void Advance(int num)
    {
        cur_ind+=num;
    }
};

struct OutFile
{
    FILE* file;
    OutFile(const char* str) {file=0; if(str) file=fopen(str, "w");}
    ~OutFile(){if(file) fclose(file);}

    void Out(const char* s)
    {
        fprintf(file, "%s\n", s); fflush(file);
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Compiler Parameters /////////////////////////////////////////////////////////////

struct CompilerInfo
{
    InFile in_file;
    OutFile out_file;
    OutFile debug_file;

    CompilerInfo(const char* in_str, const char* out_str, const char* debug_str)
                : in_file(in_str), out_file(out_str), debug_file(debug_str)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Scanner /////////////////////////////////////////////////////////////////////////

#define MAX_TOKEN_LEN 40

enum TokenType{
                IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE,
                ASSIGN, EQUAL, LESS_THAN,
                PLUS, MINUS, TIMES, DIVIDE, POWER,
                SEMI_COLON,
                LEFT_PAREN, RIGHT_PAREN,
                LEFT_BRACE, RIGHT_BRACE,
                ID, NUM,
                ENDFILE, ERROR,
                AND_OPER,
                INT_TYPE, REAL_TYPE, BOOL_TYPE  // this line
              };

// Used for debugging only /////////////////////////////////////////////////////////
const char* TokenTypeStr[]=
            {
                "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
                "Assign", "Equal", "LessThan",
                "Plus", "Minus", "Times", "Divide", "Power",
                "SemiColon",
                "LeftParen", "RightParen",
                "LeftBrace", "RightBrace",
                "ID", "Num",
                "EndFile", "Error",
                "AndOper",
                "IntType", "RealType", "BoolType"  //this line
            };

struct Token
{
    TokenType type;
    char str[MAX_TOKEN_LEN+1];

    Token(){str[0]=0; type=ERROR;}
    Token(TokenType _type, const char* _str) {type=_type; Copy(str, _str);}
};

const Token reserved_words[]=
{
    Token(IF, "if"),
    Token(THEN, "then"),
    Token(ELSE, "else"),
    Token(END, "end"),
    Token(REPEAT, "repeat"),
    Token(UNTIL, "until"),
    Token(READ, "read"),
    Token(WRITE, "write"),
    Token(INT_TYPE, "int"),       // from this  
    Token(REAL_TYPE, "real"),
    Token(BOOL_TYPE, "bool")      //to this line
};
const int num_reserved_words=sizeof(reserved_words)/sizeof(reserved_words[0]);

// the closing comment should come immediately after opening comment
const Token symbolic_tokens[]=
{
    Token(ASSIGN, ":="),
    Token(EQUAL, "="),
    Token(LESS_THAN, "<"),
    Token(PLUS, "+"),
    Token(MINUS, "-"),
    Token(TIMES, "*"),
    Token(DIVIDE, "/"),
    Token(POWER, "^"),
    Token(SEMI_COLON, ";"),
    Token(LEFT_PAREN, "("),
    Token(RIGHT_PAREN, ")"),
    Token(LEFT_BRACE, "{"),
    Token(RIGHT_BRACE, "}"),
    Token(AND_OPER, "&")
};
const int num_symbolic_tokens=sizeof(symbolic_tokens)/sizeof(symbolic_tokens[0]);

inline bool IsDigit(char ch){return (ch>='0' && ch<='9');}
inline bool IsLetter(char ch){return ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z'));}
inline bool IsLetterOrUnderscore(char ch){return (IsLetter(ch) || ch=='_');}

void GetNextToken(CompilerInfo* pci, Token* ptoken)
{
    ptoken->type=ERROR;
    ptoken->str[0]=0;

    int i;
    char* s=pci->in_file.GetNextTokenStr();
    if(!s)
    {
        ptoken->type=ENDFILE;
        ptoken->str[0]=0;
        return;
    }

    for(i=0;i<num_symbolic_tokens;i++)
    {
        if(StartsWith(s, symbolic_tokens[i].str))
            break;
    }

    if(i<num_symbolic_tokens)
    {
        if(symbolic_tokens[i].type==LEFT_BRACE)
        {
            pci->in_file.Advance(strlen(symbolic_tokens[i].str));
            if(!pci->in_file.SkipUpto(symbolic_tokens[i+1].str)) return;
            return GetNextToken(pci, ptoken);
        }
        ptoken->type=symbolic_tokens[i].type;
        Copy(ptoken->str, symbolic_tokens[i].str);
    }
else if (IsDigit(s[0]))               //from this line
{
    int j = 1;
    bool is_real = false;

    // Read digits before decimal point
    while (IsDigit(s[j])) j++;

    // Check for decimal point
    if (s[j] == '.')
    {
        is_real = true;
        j++;
        while (IsDigit(s[j])) j++;
    }

    if (is_real)
        ptoken->type = REAL_TYPE;
    else
        ptoken->type = INT_TYPE;

    Copy(ptoken->str, s, j);
}                                     //to this line
    else if(IsLetterOrUnderscore(s[0]))
    {
        int j=1;
        while(IsLetterOrUnderscore(s[j])) j++;

        ptoken->type=ID;
        Copy(ptoken->str, s, j);

        for(i=0;i<num_reserved_words;i++)
        {
            if(Equals(ptoken->str, reserved_words[i].str))
            {
                ptoken->type=reserved_words[i].type;
                break;
            }
        }
    }

    int len=strlen(ptoken->str);
    if(len>0) pci->in_file.Advance(len);
}

////////////////////////////////////////////////////////////////////////////////////
// Parser //////////////////////////////////////////////////////////////////////////

// program -> stmtseq
// stmtseq -> stmt { ; stmt }
// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt
// ifstmt -> if exp then stmtseq [ else stmtseq ] end
// repeatstmt -> repeat stmtseq until expr
// assignstmt -> identifier := expr
// readstmt -> read identifier
// writestmt -> write expr
// expr -> mathexpr [ (<|=) mathexpr ]
// mathexpr -> term { (+|-) term }    left associative
// term -> ampersand_term { (*|/) ampersand_term}   left associative
// ampersand_term -> factor { (& factor }    left associative
// factor -> newexpr { ^ newexpr }    right associative
// newexpr -> ( mathexpr ) | number | identifier

enum NodeKind{
                IF_NODE, REPEAT_NODE, ASSIGN_NODE, READ_NODE, WRITE_NODE,
                OPER_NODE, NUM_NODE, ID_NODE, DECLARE_NODE                        // add declare node
             };

// Used for debugging only /////////////////////////////////////////////////////////
const char* NodeKindStr[]=
            {
                "If", "Repeat", "Assign", "Read", "Write",
                "Oper", "Num", "ID" , "Decl"                      //add decl for debugging
            };

enum ExprDataType {VOID, INTEGER, REAL , BOOLEAN};                // add REAL expression data type

// Used for debugging only /////////////////////////////////////////////////////////
const char* ExprDataTypeStr[]=
            {
                "Void", "Integer" ,"Real", "Boolean"                    //add real expr for debugging
            };

#define MAX_CHILDREN 3

struct TreeNode
{
    TreeNode* child[MAX_CHILDREN];
    TreeNode* sibling; // used for sibling statements only

    NodeKind node_kind;

    union{TokenType oper; int num; char* id; double real_num;};                // Add real_num
    ExprDataType expr_data_type; // defined for expression/int/identifier only
    ExprDataType var_type;                                  // Add for variable declarations

    int line_num;

    TreeNode() {
        int i; 
        for(i=0;i<MAX_CHILDREN;i++) child[i]=0; 
        sibling=0; 
        expr_data_type=VOID; 
        var_type=VOID;
        num=0;  // Initialize union to zero
    }
};

struct ParseInfo
{
    Token next_token;
};

void Match(CompilerInfo* pci, ParseInfo* ppi, TokenType expected_token_type)
{
    pci->debug_file.Out("Start Match");

    if(ppi->next_token.type!=expected_token_type) throw 0;
    GetNextToken(pci, &ppi->next_token);

    fprintf(pci->debug_file.file, "[%d] %s (%s)\n", pci->in_file.cur_line_num, ppi->next_token.str, TokenTypeStr[ppi->next_token.type]); fflush(pci->debug_file.file);
}

TreeNode* MathExpr(CompilerInfo*, ParseInfo*);

// newexpr -> ( mathexpr ) | number | identifier
TreeNode* NewExpr(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start NewExpr");

    // Compare the next token with the First() of possible statements
    if(ppi->next_token.type==REAL_TYPE || ppi->next_token.type==INT_TYPE )         // from this
    {
        TreeNode* tree=new TreeNode;
        tree->node_kind=NUM_NODE;
        char* num_str=ppi->next_token.str;
        
        // Check if it's a real number (contains decimal point)
        bool is_real = false;
        if(ppi->next_token.type==REAL_TYPE ){
            is_real = true;
        }
        
        if(is_real) {
            tree->real_num = atof(num_str);  
            tree->expr_data_type = REAL;
        } else {
            tree->num = atoi(num_str);  
            tree->expr_data_type = INTEGER;
        }                                                               // to this
        // printf("(DEBUG: expr_data_type=%d, REAL=%d, num=%d, real_num=%g)", node->expr_data_type, REAL, node->num, node->real_num);

        
        tree->line_num=pci->in_file.cur_line_num;
        Match(pci, ppi, ppi->next_token.type);

        pci->debug_file.Out("End NewExpr");
        return tree;
    }
    if(ppi->next_token.type==ID)
    {
        TreeNode* tree=new TreeNode;
        tree->node_kind=ID_NODE;
        AllocateAndCopy(&tree->id, ppi->next_token.str);
        tree->line_num=pci->in_file.cur_line_num;
        Match(pci, ppi, ppi->next_token.type);

        pci->debug_file.Out("End NewExpr");
        return tree;
    }

    if(ppi->next_token.type==LEFT_PAREN)
    {
        Match(pci, ppi, LEFT_PAREN);
        TreeNode* tree=MathExpr(pci, ppi);
        Match(pci, ppi, RIGHT_PAREN);

        pci->debug_file.Out("End NewExpr");
        return tree;
    }

    throw "Unexpected token found";
    return 0;
}

// factor -> newexpr { ^ newexpr }    right associative
TreeNode* Factor(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start Factor");

    TreeNode* tree=NewExpr(pci, ppi);

    if(ppi->next_token.type==POWER)
    {
        TreeNode* new_tree=new TreeNode;
        new_tree->node_kind=OPER_NODE;
        new_tree->oper=ppi->next_token.type;
        new_tree->line_num=pci->in_file.cur_line_num;

        new_tree->child[0]=tree;
        Match(pci, ppi, ppi->next_token.type);
        new_tree->child[1]=Factor(pci, ppi);

        pci->debug_file.Out("End Factor");
        return new_tree;
    }
    pci->debug_file.Out("End Factor");
    return tree;
}
// ampersand_term -> factor { (& factor }    left associative
TreeNode* AmpersandTerm(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start AmpersandTerm");
    TreeNode* tree=Factor(pci, ppi);
    
    while(ppi->next_token.type== AND_OPER)
    {
        TreeNode* new_tree = new TreeNode;
        new_tree->node_kind = OPER_NODE;
        new_tree->oper = ppi->next_token.type;
        new_tree->line_num = pci->in_file.cur_line_num;

        new_tree->child[0] = tree;
        Match(pci, ppi, ppi->next_token.type);
        new_tree->child[1] = Factor(pci, ppi);

        tree = new_tree;
    }
    pci->debug_file.Out("End AmpersandTerm");
    return tree;
}

// term -> ampersand_term { (*|/) ampersand_term}   left associative
TreeNode* Term(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start Term");

    TreeNode* tree=AmpersandTerm(pci, ppi);

    while(ppi->next_token.type==TIMES || ppi->next_token.type==DIVIDE)
    {
        TreeNode* new_tree=new TreeNode;
        new_tree->node_kind=OPER_NODE;
        new_tree->oper=ppi->next_token.type;
        new_tree->line_num=pci->in_file.cur_line_num;

        new_tree->child[0]=tree;
        Match(pci, ppi, ppi->next_token.type);
        new_tree->child[1]=AmpersandTerm(pci, ppi);

        tree=new_tree;
    }
    pci->debug_file.Out("End Term");
    return tree;
}



// mathexpr -> term { (+|-) term }    left associative
TreeNode* MathExpr(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start MathExpr");

    TreeNode* tree=Term(pci, ppi);

    while(ppi->next_token.type==PLUS || ppi->next_token.type==MINUS)
    {
        TreeNode* new_tree=new TreeNode;
        new_tree->node_kind=OPER_NODE;
        new_tree->oper=ppi->next_token.type;
        new_tree->line_num=pci->in_file.cur_line_num;

        new_tree->child[0]=tree;
        Match(pci, ppi, ppi->next_token.type);
        new_tree->child[1]=Term(pci, ppi);

        tree=new_tree;
    }
    pci->debug_file.Out("End MathExpr");
    return tree;
}

// expr -> mathexpr [ (<|=) mathexpr ]
TreeNode* Expr(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start Expr");

    TreeNode* tree=MathExpr(pci, ppi);

    if(ppi->next_token.type==EQUAL || ppi->next_token.type==LESS_THAN)
    {
        TreeNode* new_tree=new TreeNode;
        new_tree->node_kind=OPER_NODE;
        new_tree->oper=ppi->next_token.type;
        new_tree->line_num=pci->in_file.cur_line_num;

        new_tree->child[0]=tree;
        Match(pci, ppi, ppi->next_token.type);
        new_tree->child[1]=MathExpr(pci, ppi);

        pci->debug_file.Out("End Expr");
        return new_tree;
    }
    pci->debug_file.Out("End Expr");
    return tree;
}

// writestmt -> write expr
TreeNode* WriteStmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start WriteStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=WRITE_NODE;
    tree->line_num=pci->in_file.cur_line_num;

    Match(pci, ppi, WRITE);
    tree->child[0]=Expr(pci, ppi);

    pci->debug_file.Out("End WriteStmt");
    return tree;
}

// readstmt -> read identifier
TreeNode* ReadStmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start ReadStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=READ_NODE;
    tree->line_num=pci->in_file.cur_line_num;

    Match(pci, ppi, READ);
    if(ppi->next_token.type==ID) AllocateAndCopy(&tree->id, ppi->next_token.str);
    Match(pci, ppi, ID);

    pci->debug_file.Out("End ReadStmt");
    return tree;
}

// assignstmt -> identifier := expr
TreeNode* AssignStmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start AssignStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=ASSIGN_NODE;
    tree->line_num=pci->in_file.cur_line_num;

    if(ppi->next_token.type==ID) AllocateAndCopy(&tree->id, ppi->next_token.str);
    Match(pci, ppi, ID);
    Match(pci, ppi, ASSIGN); tree->child[0]=Expr(pci, ppi);

    pci->debug_file.Out("End AssignStmt");
    return tree;
}

TreeNode* StmtSeq(CompilerInfo*, ParseInfo*);

// repeatstmt -> repeat stmtseq until expr
TreeNode* RepeatStmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start RepeatStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=REPEAT_NODE;
    tree->line_num=pci->in_file.cur_line_num;

    Match(pci, ppi, REPEAT); tree->child[0]=StmtSeq(pci, ppi);
    Match(pci, ppi, UNTIL); tree->child[1]=Expr(pci, ppi);

    pci->debug_file.Out("End RepeatStmt");
    return tree;
}

// ifstmt -> if exp then stmtseq [ else stmtseq ] end
TreeNode* IfStmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start IfStmt");

    TreeNode* tree=new TreeNode;
    tree->node_kind=IF_NODE;
    tree->line_num=pci->in_file.cur_line_num;

    Match(pci, ppi, IF); tree->child[0]=Expr(pci, ppi);
    Match(pci, ppi, THEN); tree->child[1]=StmtSeq(pci, ppi);
    if(ppi->next_token.type==ELSE) {Match(pci, ppi, ELSE); tree->child[2]=StmtSeq(pci, ppi);}
    Match(pci, ppi, END);

    pci->debug_file.Out("End IfStmt");
    return tree;
}

// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt
TreeNode* Stmt(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start Stmt");

    // Compare the next token with the First() of possible statements
    TreeNode* tree=0;
    if(ppi->next_token.type==IF) tree=IfStmt(pci, ppi);
    else if(ppi->next_token.type==REPEAT) tree=RepeatStmt(pci, ppi);
    else if(ppi->next_token.type==ID) tree=AssignStmt(pci, ppi);
    else if(ppi->next_token.type==READ) tree=ReadStmt(pci, ppi);
    else if(ppi->next_token.type==WRITE) tree=WriteStmt(pci, ppi);
    else throw 0;

    pci->debug_file.Out("End Stmt");
    return tree;
}

// declaration -> type identifier
TreeNode* Declaration(CompilerInfo* pci, ParseInfo* ppi)           // from this
{
    pci->debug_file.Out("Start Declaration");

    TreeNode* tree = new TreeNode;
    tree->node_kind = DECLARE_NODE;
    tree->line_num = pci->in_file.cur_line_num;

    // Get the type
    if(ppi->next_token.type == INT_TYPE) {
        tree->var_type = INTEGER;
        Match(pci, ppi, INT_TYPE);
    }
    else if(ppi->next_token.type == REAL_TYPE) {
        tree->var_type = REAL;
        Match(pci, ppi, REAL_TYPE);
    }
    else if(ppi->next_token.type == BOOL_TYPE) {
        tree->var_type = BOOLEAN;
        Match(pci, ppi, BOOL_TYPE);
    }
    else {
        throw "Unexpected type found!"; 
    }                                                              // to this

    // Get the identifier
    if(ppi->next_token.type == ID) {
        AllocateAndCopy(&tree->id, ppi->next_token.str);
    }
    Match(pci, ppi, ID);

    pci->debug_file.Out("End Declaration");
    return tree;
}
// declarations -> declaration { ; declaration }  , must be after each other (like at first lines as the grammer rule)
TreeNode* Declarations(CompilerInfo* pci, ParseInfo* ppi)            //from this
{
    pci->debug_file.Out("Start Declarations");

    TreeNode* first_tree = Declaration(pci, ppi);
    TreeNode* last_tree = first_tree;

    // Continue parsing declarations while we see semicolons followed by type keywords
    while(ppi->next_token.type == SEMI_COLON)
    {
        // Look ahead to see if next is a type keyword
        Token saved_token = ppi->next_token;
        Match(pci, ppi, SEMI_COLON);
        
        // Check if this is a declaration or start of statements
        if(ppi->next_token.type == INT_TYPE || 
           ppi->next_token.type == REAL_TYPE || 
           ppi->next_token.type == BOOL_TYPE)
        {
            TreeNode* next_tree = Declaration(pci, ppi);
            last_tree->sibling = next_tree;
            last_tree = next_tree;
        }
        else
        {
            // Not a declaration, we're done with declarations
            break;
        }
    }

    pci->debug_file.Out("End Declarations");
    return first_tree;
}                                                                           //to this line
// stmtseq -> stmt { ; stmt }
TreeNode* StmtSeq(CompilerInfo* pci, ParseInfo* ppi)
{
    pci->debug_file.Out("Start StmtSeq");
    TreeNode* first_tree=Stmt(pci, ppi);
    TreeNode* last_tree=first_tree;
    
    while(ppi->next_token.type!=ENDFILE && 
          ppi->next_token.type!=END && 
          ppi->next_token.type!=ELSE && 
          ppi->next_token.type!=UNTIL)
    {
        Match(pci, ppi, SEMI_COLON);
        
        // Check if there's actually another statement to parse
        if(ppi->next_token.type==ENDFILE || 
           ppi->next_token.type==END || 
           ppi->next_token.type==ELSE || 
           ppi->next_token.type==UNTIL)
            break;
            
        TreeNode* next_tree=Stmt(pci, ppi);
        last_tree->sibling=next_tree;
        last_tree=next_tree;
    }
    
    pci->debug_file.Out("End StmtSeq");
    return first_tree;
}

// program -> stmtseq
// program -> declarations stmtseq
TreeNode* Parse(CompilerInfo* pci)                                    //from this
{
    ParseInfo parse_info;
    GetNextToken(pci, &parse_info.next_token);

    TreeNode* syntax_tree = 0;
    TreeNode* declarations_tree = 0;
    TreeNode* statements_tree = 0;

    // Check if we have declarations (starts with type keyword)
    if(parse_info.next_token.type == INT_TYPE || 
       parse_info.next_token.type == REAL_TYPE || 
       parse_info.next_token.type == BOOL_TYPE)
    {
        declarations_tree = Declarations(pci, &parse_info);
    }

    // Now parse statements
    if(parse_info.next_token.type != ENDFILE)
    {
        statements_tree = StmtSeq(pci, &parse_info);
    }

    // Link declarations and statements
    if(declarations_tree)
    {
        syntax_tree = declarations_tree;
        // Find last declaration
        TreeNode* last_decl = declarations_tree;
        while(last_decl->sibling) last_decl = last_decl->sibling;
        last_decl->sibling = statements_tree;
    }
    else
    {
        syntax_tree = statements_tree;
    }

    if(parse_info.next_token.type != ENDFILE)
        pci->debug_file.Out("Error code ends before file ends");

    return syntax_tree;
}                                                                     // to this line

void PrintTree(TreeNode* node, int sh=0)
{
    int i, NSH=3;
    for(i=0;i<sh;i++) printf(" ");

    printf("[%s]", NodeKindStr[node->node_kind]);

    if(node->node_kind==OPER_NODE) printf("[%s]", TokenTypeStr[node->oper]);
    else if(node->node_kind==NUM_NODE) {
        // IMPORTANT: Check the type FIRST before printing
        if(node->expr_data_type == REAL) {
            printf("[%g]", node->real_num);  // Print as real
        } else {
            printf("[%d]", node->num);       // Print as integer
        }
    }
    else if(node->node_kind==ID_NODE || node->node_kind==READ_NODE || 
            node->node_kind==ASSIGN_NODE || node->node_kind==DECLARE_NODE) {
        if(node->id) printf("[%s]", node->id);  // Add null check
    }

    // Print expression data type
    if(node->expr_data_type!=VOID) printf("[%s]", ExprDataTypeStr[node->expr_data_type]);
    
    // Print variable type for declarations
    if(node->node_kind==DECLARE_NODE && node->var_type!=VOID) {
        printf("[%s]", ExprDataTypeStr[node->var_type]);
    }

    printf("\n");

    for(i=0;i<MAX_CHILDREN;i++) if(node->child[i]) PrintTree(node->child[i], sh+NSH);
    if(node->sibling) PrintTree(node->sibling, sh);
}

void DestroyTree(TreeNode* node)
{
    int i;

    if(node->node_kind==ID_NODE || node->node_kind==READ_NODE || 
       node->node_kind==ASSIGN_NODE || node->node_kind==DECLARE_NODE)                                                // ADD DECL_NODE
        if(node->id) delete[] node->id;

    for(i=0;i<MAX_CHILDREN;i++) if(node->child[i]) DestroyTree(node->child[i]);
    if(node->sibling) DestroyTree(node->sibling);

    delete node;
}

////////////////////////////////////////////////////////////////////////////////////
// Analyzer ////////////////////////////////////////////////////////////////////////

const int SYMBOL_HASH_SIZE=10007;

struct LineLocation
{
    int line_num;
    LineLocation* next;
};

struct VariableInfo
{
    char* name;
    int memloc;
    LineLocation* head_line; // the head of linked list of source line locations
    LineLocation* tail_line; // the tail of linked list of source line locations
    VariableInfo* next_var; // the next variable in the linked list in the same hash bucket of the symbol table
    ExprDataType var_type;                                                                                           // ADD THIS
};

struct SymbolTable
{
    int num_vars;
    VariableInfo* var_info[SYMBOL_HASH_SIZE];

    SymbolTable() {num_vars=0; int i; for(i=0;i<SYMBOL_HASH_SIZE;i++) var_info[i]=0;}

    int Hash(const char* name)
    {
        int i, len=strlen(name);
        int hash_val=11;
        for(i=0;i<len;i++) hash_val=(hash_val*17+(int)name[i])%SYMBOL_HASH_SIZE;
        return hash_val;
    }

    VariableInfo* Find(const char* name)
    {
        int h=Hash(name);
        VariableInfo* cur=var_info[h];
        while(cur)
        {
            if(Equals(name, cur->name)) return cur;
            cur=cur->next_var;
        }
        return 0;
    }

    void Insert(const char* name, int line_num ,ExprDataType type = VOID)
    {
        LineLocation* lineloc=new LineLocation;
        lineloc->line_num=line_num;
        lineloc->next=0;

        int h=Hash(name);
        VariableInfo* prev=0;
        VariableInfo* cur=var_info[h];

        while(cur)
        {
            if(Equals(name, cur->name))
            {
                // just add this line location to the list of line locations of the existing var
                cur->tail_line->next=lineloc;
                cur->tail_line=lineloc;
                return;
            }
            prev=cur;
            cur=cur->next_var;
        }

        VariableInfo* vi=new VariableInfo;
        vi->head_line=vi->tail_line=lineloc;
        vi->next_var=0;
        vi->memloc=num_vars++;
        vi->var_type=type;                                           //  add variable type
        AllocateAndCopy(&vi->name, name);

        if(!prev) var_info[h]=vi;
        else prev->next_var=vi;
    }

    void Print()
    {
        int i;
        for(i=0;i<SYMBOL_HASH_SIZE;i++)
        {
            VariableInfo* curv=var_info[i];
            while(curv)
            {
                printf("[Var=%s][Mem=%d]", curv->name, curv->memloc);
                LineLocation* curl=curv->head_line;
                while(curl)
                {
                    printf("[Line=%d]", curl->line_num);
                    curl=curl->next;
                }
                printf("\n");
                curv=curv->next_var;
            }
        }
    }

    void Destroy()
    {
        int i;
        for(i=0;i<SYMBOL_HASH_SIZE;i++)
        {
            VariableInfo* curv=var_info[i];
            while(curv)
            {
                LineLocation* curl=curv->head_line;
                while(curl)
                {
                    LineLocation* pl=curl;
                    curl=curl->next;
                    delete pl;
                }
                VariableInfo* p=curv;
                curv=curv->next_var;
                delete p;
            }
            var_info[i]=0;
        }
    }
};
void Analyze(TreeNode* node, SymbolTable* symbol_table)
{
    int i;

    // Handle declarations - register variables with their types
    if(node->node_kind == DECLARE_NODE) {
        symbol_table->Insert(node->id, node->line_num, node->var_type);
    }
    
    if(node->node_kind==ID_NODE || node->node_kind==READ_NODE || node->node_kind==ASSIGN_NODE) {
        VariableInfo* var = symbol_table->Find(node->id);
        if(!var) {
            printf("ERROR: Variable '%s' used but not declared at line %d\n", node->id, node->line_num);
            throw "Terminate Program!";
        }
        symbol_table->Insert(node->id, node->line_num);
    }

    for(i=0;i<MAX_CHILDREN;i++) 
        if(node->child[i]) 
            Analyze(node->child[i], symbol_table);

    if(node->node_kind==OPER_NODE)             
    {
        if(node->oper==EQUAL || node->oper==LESS_THAN) {
            // Comparison operators - check operands are int or real (not bool)
            if(node->child[0]->expr_data_type == BOOLEAN || node->child[1]->expr_data_type == BOOLEAN) {
                printf("ERROR: Cannot use comparison operators on BOOLEAN at line %d\n", node->line_num);
                throw "Terminate Program!";
            }
            node->expr_data_type=BOOLEAN;
        }
        else if(node->oper==AND_OPER) {
            // AND operator only for integers
            if(node->child[0]->expr_data_type != INTEGER || node->child[1]->expr_data_type != INTEGER) {
                printf("ERROR: AND operator requires INTEGER operands at line %d\n", node->line_num);
                throw "Terminate Program!";
            }
            node->expr_data_type=INTEGER;
        }
        else {
            // Arithmetic operators - check not boolean
            if(node->child[0]->expr_data_type == BOOLEAN || node->child[1]->expr_data_type == BOOLEAN) {
                printf("ERROR: Cannot do arithmetic on BOOLEAN at line %d\n", node->line_num);
                throw "Terminate Program!";
            }
            
            // Type promotion: if either operand is REAL, result is REAL
            if(node->child[0]->expr_data_type == REAL || node->child[1]->expr_data_type == REAL)
                node->expr_data_type = REAL;
            else
                node->expr_data_type = INTEGER;
        }
    }
    else if(node->node_kind==ID_NODE) {
        VariableInfo* var = symbol_table->Find(node->id);
        if(var) node->expr_data_type = var->var_type;
    }

    // Type checking for statements
    if(node->node_kind==IF_NODE) {
        if(node->child[0]->expr_data_type != BOOLEAN) {
            printf("ERROR: If condition must be BOOLEAN at line %d\n", node->line_num);
            throw "Terminate Program!";
        }
    }
    
    if(node->node_kind==REPEAT_NODE) {
        if(node->child[1]->expr_data_type != BOOLEAN) {
            printf("ERROR: Repeat condition must be BOOLEAN at line %d\n", node->line_num);
            throw "Terminate Program!";
        }
    }
    
    if(node->node_kind==ASSIGN_NODE) {
        VariableInfo* var = symbol_table->Find(node->id);
        if(var && var->var_type != node->child[0]->expr_data_type) {
            printf("ERROR: Cannot assign %s to %s variable '%s' at line %d\n", 
                   ExprDataTypeStr[node->child[0]->expr_data_type],
                   ExprDataTypeStr[var->var_type],
                   node->id, node->line_num);
            throw "Terminate Program!";
        }
    }

    if(node->sibling) Analyze(node->sibling, symbol_table);
}

////////////////////////////////////////////////////////////////////////////////////
// Code Generator //////////////////////////////////////////////////////////////////

struct Variable {
    ExprDataType type;
    union {
        int int_val;
        double real_val;
        bool bool_val;
    };
    
    Variable() {
        type = VOID;
        int_val = 0;  // Initialize union
    }
};

int Power(int a, int b)
{
    if(a==0) return 0;
    if(b==0) return 1;
    if(b>=1) return a*Power(a, b-1);
    return 0;
}

//interpreter
// Unified evaluation function that handles all types
double EvaluateReal(TreeNode* node, SymbolTable* symbol_table, Variable* variables)
{
    // Base cases: NUM_NODE and ID_NODE
    if(node->node_kind == NUM_NODE) {
        if(node->expr_data_type == REAL) 
            return node->real_num;
        else 
            return (double)node->num;
    }
    
    if(node->node_kind == ID_NODE) {
        VariableInfo* var_info = symbol_table->Find(node->id);
        if(!var_info) {
            printf("ERROR: Variable '%s' not found\n", node->id);
            throw "Terminate Program!";
        }
        int memloc = var_info->memloc;
        
        if(variables[memloc].type == REAL) 
            return variables[memloc].real_val;
        else if(variables[memloc].type == INTEGER) 
            return (double)variables[memloc].int_val;
        else if(variables[memloc].type == BOOLEAN)
            return variables[memloc].bool_val ? 1.0 : 0.0;
        
        return 0.0;
    }

    // Recursive evaluation for operators
    double a = EvaluateReal(node->child[0], symbol_table, variables);
    double b = EvaluateReal(node->child[1], symbol_table, variables);

    if(node->oper == EQUAL) return (a == b) ? 1.0 : 0.0;
    if(node->oper == LESS_THAN) return (a < b) ? 1.0 : 0.0;
    if(node->oper == PLUS) return a + b;
    if(node->oper == MINUS) return a - b;
    if(node->oper == TIMES) return a * b;
    if(node->oper == DIVIDE) return a / b;
    if(node->oper == POWER) return pow(a, b);  
    if(node->oper == AND_OPER) return (a * a) - (b * b);
    
    return 0.0;
}

// NEW: Updated RunProgram to handle multiple types
void RunProgram(TreeNode* node, SymbolTable* symbol_table, Variable* variables)
{
    if(!node) return;  // Safety check
    
    // Handle declarations - skip them during execution
    if(node->node_kind == DECLARE_NODE) {
        // Declarations already processed, just skip
        if(node->sibling) RunProgram(node->sibling, symbol_table, variables);
        return;
    }
    
    // IF statement
    if(node->node_kind == IF_NODE)
    {
        double cond_val = EvaluateReal(node->child[0], symbol_table, variables);
        bool cond = (cond_val != 0.0);  // Convert to boolean
        
        if(cond) 
            RunProgram(node->child[1], symbol_table, variables);
        else if(node->child[2]) 
            RunProgram(node->child[2], symbol_table, variables);
    }
    
    // ASSIGN statement
    else if(node->node_kind == ASSIGN_NODE)
    {
        VariableInfo* var_info = symbol_table->Find(node->id);
        if(!var_info) {
            printf("ERROR: Variable '%s' not declared\n", node->id);
            throw "Terminate Program!";
        }
        
        int memloc = var_info->memloc;
        double eval_result = EvaluateReal(node->child[0], symbol_table, variables);
        
        if(variables[memloc].type == REAL) {
            variables[memloc].real_val = eval_result;
        }
        else if(variables[memloc].type == INTEGER) {
            variables[memloc].int_val = (int)eval_result;
        }
        else if(variables[memloc].type == BOOLEAN) {
            variables[memloc].bool_val = (eval_result != 0.0);
        }
    }
    
    // READ statement
    else if(node->node_kind == READ_NODE)
    {
        VariableInfo* var_info = symbol_table->Find(node->id);
        if(!var_info) {
            printf("ERROR: Variable '%s' not declared\n", node->id);
            throw "Terminate Program!";
        }
        
        int memloc = var_info->memloc;
        printf("Enter %s: ", node->id);
        
        if(variables[memloc].type == REAL) {
            scanf("%lf", &variables[memloc].real_val);
        }
        else if(variables[memloc].type == INTEGER) {
            scanf("%d", &variables[memloc].int_val);
        }
        else if(variables[memloc].type == BOOLEAN) {
            int temp;
            scanf("%d", &temp);
            variables[memloc].bool_val = (temp != 0);
        }
    }
    
    // WRITE statement
    else if(node->node_kind == WRITE_NODE)
    {
        if(node->child[0]->expr_data_type == REAL) {
            double v = EvaluateReal(node->child[0], symbol_table, variables);
            printf("Val: %g\n", v);
        }
        else if(node->child[0]->expr_data_type == INTEGER) {
            int v = (int)EvaluateReal(node->child[0], symbol_table, variables);
            printf("Val: %d\n", v);
        }
        else if(node->child[0]->expr_data_type == BOOLEAN) {
            double v = EvaluateReal(node->child[0], symbol_table, variables);
            bool b = (v != 0.0);
            printf("Val: %s\n", b ? "true" : "false");
        }
    }
    
    // REPEAT statement
    else if(node->node_kind == REPEAT_NODE)
    {
        do {
            RunProgram(node->child[0], symbol_table, variables);
        } while(EvaluateReal(node->child[1], symbol_table, variables) == 0.0);
    }
    
    // Process sibling statements
    if(node->sibling) 
        RunProgram(node->sibling, symbol_table, variables);
}
// NEW: Updated entry point for RunProgram
void RunProgram(TreeNode* syntax_tree, SymbolTable* symbol_table)
{
    int i;
    
    // Allocate array of Variable structures instead of just ints
    Variable* variables = new Variable[symbol_table->num_vars];
    
    // Initialize all variables based on their declared types
    for(i = 0; i < SYMBOL_HASH_SIZE; i++)
    {
        VariableInfo* curv = symbol_table->var_info[i];
        while(curv)
        {
            variables[curv->memloc].type = curv->var_type;
            
            if(curv->var_type == INTEGER)
                variables[curv->memloc].int_val = 0;
            else if(curv->var_type == REAL)
                variables[curv->memloc].real_val = 0.0;
            else if(curv->var_type == BOOLEAN)
                variables[curv->memloc].bool_val = false;
            
            curv = curv->next_var;
        }
    }
    
    // Run the program
    RunProgram(syntax_tree, symbol_table, variables);
    
    // Clean up
    delete[] variables;
}


////////////////////////////////////////////////////////////////////////////////////
// Scanner and Compiler ////////////////////////////////////////////////////////////

void StartCompiler(CompilerInfo* pci)
{
    TreeNode* syntax_tree=Parse(pci);

    SymbolTable symbol_table;
    Analyze(syntax_tree, &symbol_table);

    printf("Symbol Table:\n");
    symbol_table.Print();
    printf("---------------------------------\n"); fflush(NULL);

    printf("Syntax Tree:\n");
    PrintTree(syntax_tree);
    printf("---------------------------------\n"); fflush(NULL);

    printf("Run Program:\n");
    RunProgram(syntax_tree, &symbol_table);
    printf("---------------------------------\n"); fflush(NULL);

    symbol_table.Destroy();
    DestroyTree(syntax_tree);
}

////////////////////////////////////////////////////////////////////////////////////
// Scanner only ////////////////////////////////////////////////////////////////////

void StartScanner(CompilerInfo* pci)
{
    Token token;

    while(true)
    {
        GetNextToken(pci, &token);
        printf("[%d] %s (%s)\n", pci->in_file.cur_line_num, token.str, TokenTypeStr[token.type]); fflush(NULL);
        if(token.type==ENDFILE || token.type==ERROR) break;
    }
}

////////////////////////////////////////////////////////////////////////////////////

int main()
{
    printf("Start main()\n"); fflush(NULL);

    CompilerInfo compiler_info("input.txt", "output.txt", "debug.txt");

    // StartScanner(&compiler_info);
    StartCompiler(&compiler_info);  

    printf("End main()\n"); fflush(NULL);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
