#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <thread>
#include <chrono>

// Token types for the lexer
enum TokenType {
    IDENTIFIER,
    NUMBER,
    STRING,
    KEYWORD,
    OPERATOR,
    DELIMITER,
    COMMENT,
    END_OF_FILE
};

// Data types
enum DataType {
    INT_TYPE,
    FLOAT_TYPE,
    BOOL_TYPE,
    STRING_TYPE,
    VOID_TYPE
};

// Token structure
struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType t, const std::string& v, int l = 0, int c = 0) 
        : type(t), value(v), line(l), column(c) {}
};

// Variable structure
struct Variable {
    DataType type;
    std::string value;
    
    Variable() : type(INT_TYPE), value("0") {}
    Variable(DataType t, const std::string& v) : type(t), value(v) {}
};

// Function structure
struct Function {
    DataType returnType;
    std::vector<std::string> parameters;
    std::vector<Token> body;
    
    Function() : returnType(VOID_TYPE) {}
    Function(DataType rt, const std::vector<std::string>& params) 
        : returnType(rt), parameters(params) {}
};

class CrimsonInterpreter {
private:
    std::map<std::string, Variable> variables;
    std::map<std::string, Function> functions;
    std::vector<Token> tokens;
    size_t currentToken;
    std::string currentFile;
    
    // Keywords
    std::set<std::string> keywords = {
        "int", "float", "bool", "string", "void", "if", "else", "switch", 
        "main", "include", "true", "false", "else if"
    };
    
    // Built-in functions
    std::set<std::string> builtinFunctions = {
        "crym", "inp", "Sleep"
    };

public:
    CrimsonInterpreter() : currentToken(0) {}
    
    // Lexer
    std::vector<Token> tokenize(const std::string& code) {
        std::vector<Token> tokens;
        std::stringstream ss(code);
        std::string line;
        int lineNum = 1;
        
        while (std::getline(ss, line)) {
            size_t pos = 0;
            while (pos < line.length()) {
                // Skip whitespace
                while (pos < line.length() && std::isspace(line[pos])) {
                    pos++;
                }
                
                if (pos >= line.length()) break;
                
                // Comments
                if (pos < line.length() - 1 && line[pos] == '/' && line[pos + 1] == '/') {
                    tokens.push_back(Token(COMMENT, line.substr(pos), lineNum, pos));
                    break;
                }
                
                // Include statements
                if (line[pos] == '#') {
                    size_t start = pos;
                    while (pos < line.length() && line[pos] != '\n') {
                        pos++;
                    }
                    tokens.push_back(Token(KEYWORD, line.substr(start, pos - start), lineNum, start));
                    continue;
                }
                
                // Strings
                if (line[pos] == '"') {
                    size_t start = pos;
                    pos++;
                    while (pos < line.length() && line[pos] != '"') {
                        if (line[pos] == '\\' && pos + 1 < line.length()) {
                            pos += 2; // Skip escaped character
                        } else {
                            pos++;
                        }
                    }
                    if (pos < line.length()) pos++; // Include closing quote
                    tokens.push_back(Token(STRING, line.substr(start, pos - start), lineNum, start));
                    continue;
                }
                
                // Numbers
                if (std::isdigit(line[pos]) || line[pos] == '.') {
                    size_t start = pos;
                    while (pos < line.length() && (std::isdigit(line[pos]) || line[pos] == '.')) {
                        pos++;
                    }
                    tokens.push_back(Token(NUMBER, line.substr(start, pos - start), lineNum, start));
                    continue;
                }
                
                // Identifiers and keywords
                if (std::isalpha(line[pos]) || line[pos] == '_') {
                    size_t start = pos;
                    while (pos < line.length() && (std::isalnum(line[pos]) || line[pos] == '_')) {
                        pos++;
                    }
                    std::string word = line.substr(start, pos - start);
                    TokenType type = (keywords.find(word) != keywords.end()) ? KEYWORD : IDENTIFIER;
                    tokens.push_back(Token(type, word, lineNum, start));
                    continue;
                }
                
                // Operators and delimiters
                char c = line[pos];
                if (c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || 
                    c == '!' || c == '<' || c == '>' || c == '&' || c == '|') {
                    size_t start = pos;
                    pos++;
                    // Check for double operators
                    if (pos < line.length() && 
                        ((c == '=' && line[pos] == '=') ||
                         (c == '!' && line[pos] == '=') ||
                         (c == '<' && line[pos] == '=') ||
                         (c == '>' && line[pos] == '=') ||
                         (c == '&' && line[pos] == '&') ||
                         (c == '|' && line[pos] == '|'))) {
                        pos++;
                    }
                    tokens.push_back(Token(OPERATOR, line.substr(start, pos - start), lineNum, start));
                    continue;
                }
                
                // Delimiters
                if (c == '(' || c == ')' || c == '{' || c == '}' || c == ';' || c == ',') {
                    tokens.push_back(Token(DELIMITER, std::string(1, c), lineNum, pos));
                    pos++;
                    continue;
                }
                
                pos++;
            }
            lineNum++;
        }
        
        tokens.push_back(Token(END_OF_FILE, "", lineNum, 0));
        return tokens;
    }
    
    // Parser and interpreter
    void parseAndExecute(const std::vector<Token>& tokens) {
        this->tokens = tokens;
        currentToken = 0;
        
        // First pass: Find main function and validate structure
        bool hasMainFunction = false;
        size_t mainStart = 0;
        size_t mainEnd = 0;
        
        // Look for main function
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].type == KEYWORD && 
                (tokens[i].value == "void" || tokens[i].value == "int")) {
                if (i + 1 < tokens.size() && tokens[i + 1].value == "main") {
                    hasMainFunction = true;
                    mainStart = i;
                    break;
                }
            }
        }
        
        if (!hasMainFunction) {
            std::cerr << "Error: No main function found. Code must be inside void main() or int main() to execute." << std::endl;
            return;
        }
        
        // Find main function boundaries
        int braceCount = 0;
        bool inMainFunction = false;
        for (size_t i = mainStart; i < tokens.size(); i++) {
            if (tokens[i].value == "main" && i + 1 < tokens.size() && tokens[i + 1].value == "(") {
                inMainFunction = true;
                continue;
            }
            
            if (inMainFunction) {
                if (tokens[i].value == "{") {
                    braceCount++;
                } else if (tokens[i].value == "}") {
                    braceCount--;
                    if (braceCount == 0) {
                        mainEnd = i;
                        break;
                    }
                }
            }
        }
        
        if (mainEnd == 0) {
            std::cerr << "Error: Main function not properly closed with }" << std::endl;
            return;
        }
        
        // Check for code outside main function
        for (size_t i = 0; i < tokens.size(); i++) {
            if (i < mainStart || i > mainEnd) {
                if (tokens[i].type == IDENTIFIER && 
                    (tokens[i].value == "crym" || tokens[i].value == "Sleep" || tokens[i].value == "inp")) {
                    std::cerr << "Error: Line " << tokens[i].line 
                              << ": Code outside main function is not allowed. All executable code must be inside main()." << std::endl;
                    return;
                }
            }
        }
        
        // Reset current token to start of main function
        currentToken = mainStart;
        
        // Parse and execute only within main function
        while (currentToken < tokens.size() && currentToken <= mainEnd && tokens[currentToken].type != END_OF_FILE) {
            if (tokens[currentToken].type == COMMENT) {
                currentToken++;
                continue;
            }
            
            if (tokens[currentToken].type == KEYWORD && tokens[currentToken].value == "#include") {
                parseInclude();
            } else if (tokens[currentToken].type == KEYWORD && 
                      (tokens[currentToken].value == "int" || 
                       tokens[currentToken].value == "float" || 
                       tokens[currentToken].value == "bool" || 
                       tokens[currentToken].value == "string")) {
                parseVariableDeclaration();
            } else if (tokens[currentToken].type == KEYWORD && tokens[currentToken].value == "void") {
                parseFunctionDeclaration();
            } else if (tokens[currentToken].type == KEYWORD && tokens[currentToken].value == "if") {
                parseIfStatement();
            } else if (tokens[currentToken].type == IDENTIFIER) {
                parseStatement();
            } else {
                currentToken++;
            }
        }
    }
    
    void parseInclude() {
        currentToken++; // Skip #include
        if (currentToken < tokens.size() && tokens[currentToken].type == DELIMITER && tokens[currentToken].value == "<") {
            currentToken++; // Skip <
            std::string libName;
            while (currentToken < tokens.size() && tokens[currentToken].type != DELIMITER) {
                libName += tokens[currentToken].value;
                currentToken++;
            }
            if (currentToken < tokens.size() && tokens[currentToken].value == ">") {
                currentToken++; // Skip >
            }
            std::cout << "Including library: " << libName << std::endl;
        }
    }
    
    void parseVariableDeclaration() {
        DataType type = getDataType(tokens[currentToken].value);
        currentToken++; // Skip type
        
        if (currentToken < tokens.size() && tokens[currentToken].type == IDENTIFIER) {
            std::string varName = tokens[currentToken].value;
            currentToken++; // Skip variable name
            
            if (currentToken < tokens.size() && tokens[currentToken].value == "=") {
                currentToken++; // Skip =
                
                std::string value = parseExpression();
                variables[varName] = Variable(type, value);
            } else {
                // Default values
                std::string defaultValue;
                switch (type) {
                    case INT_TYPE: defaultValue = "0"; break;
                    case FLOAT_TYPE: defaultValue = "0.0"; break;
                    case BOOL_TYPE: defaultValue = "false"; break;
                    case STRING_TYPE: defaultValue = ""; break;
                    default: break;
                }
                variables[varName] = Variable(type, defaultValue);
            }
            
            // Skip semicolon if present
            if (currentToken < tokens.size() && tokens[currentToken].value == ";") {
                currentToken++;
            }
        }
    }
    
    void parseFunctionDeclaration() {
        currentToken++; // Skip void
        if (currentToken < tokens.size() && tokens[currentToken].type == IDENTIFIER) {
            std::string funcName = tokens[currentToken].value;
            currentToken++; // Skip function name
            
            if (currentToken < tokens.size() && tokens[currentToken].value == "(") {
                currentToken++; // Skip (
                
                std::vector<std::string> parameters;
                while (currentToken < tokens.size() && tokens[currentToken].value != ")") {
                    if (tokens[currentToken].type == IDENTIFIER) {
                        parameters.push_back(tokens[currentToken].value);
                        currentToken++;
                    } else {
                        currentToken++;
                    }
                }
                if (currentToken < tokens.size() && tokens[currentToken].value == ")") {
                    currentToken++; // Skip )
                }
                
                if (currentToken < tokens.size() && tokens[currentToken].value == "{") {
                    currentToken++; // Skip {
                    
                    std::vector<Token> body;
                    int braceCount = 1;
                    while (currentToken < tokens.size() && braceCount > 0) {
                        if (tokens[currentToken].value == "{") braceCount++;
                        else if (tokens[currentToken].value == "}") braceCount--;
                        
                        if (braceCount > 0) {
                            body.push_back(tokens[currentToken]);
                            currentToken++;
                        }
                    }
                    if (currentToken < tokens.size() && tokens[currentToken].value == "}") {
                        currentToken++; // Skip }
                    }
                    
                    functions[funcName] = Function(VOID_TYPE, parameters);
                }
            }
        }
    }
    
    void parseStatement() {
        if (currentToken < tokens.size() && tokens[currentToken].type == IDENTIFIER) {
            std::string identifier = tokens[currentToken].value;
            currentToken++;
            
            if (currentToken < tokens.size() && tokens[currentToken].value == "(") {
                // Function call
                currentToken++; // Skip (
                
                std::vector<std::string> args;
                while (currentToken < tokens.size() && tokens[currentToken].value != ")") {
                    if (tokens[currentToken].type == STRING || tokens[currentToken].type == NUMBER) {
                        args.push_back(tokens[currentToken].value);
                        currentToken++;
                    } else if (tokens[currentToken].type == IDENTIFIER) {
                        if (variables.find(tokens[currentToken].value) != variables.end()) {
                            args.push_back(variables[tokens[currentToken].value].value);
                        } else {
                            args.push_back(tokens[currentToken].value);
                        }
                        currentToken++;
                    } else if (tokens[currentToken].value == ",") {
                        currentToken++; // Skip comma
                    } else {
                        currentToken++;
                    }
                }
                if (currentToken < tokens.size() && tokens[currentToken].value == ")") {
                    currentToken++; // Skip )
                }
                
                executeFunction(identifier, args);
                
                // Skip semicolon if present
                if (currentToken < tokens.size() && tokens[currentToken].value == ";") {
                    currentToken++;
                }
            }
        }
    }
    
    
    void executeFunction(const std::string& funcName, const std::vector<std::string>& args) {
        if (funcName == "crym") {
            if (!args.empty()) {
                std::string message = args[0];
                // Remove quotes if present
                if (message.length() >= 2 && message[0] == '"' && message.back() == '"') {
                    message = message.substr(1, message.length() - 2);
                }
                std::cout << message << std::endl;
            }
        } else if (funcName == "inp") {
            if (!args.empty()) {
                std::string prompt = args[0];
                // Remove quotes if present
                if (prompt.length() >= 2 && prompt[0] == '"' && prompt.back() == '"') {
                    prompt = prompt.substr(1, prompt.length() - 2);
                }
                std::cout << prompt;
                std::string input;
                std::getline(std::cin, input);
                // Store input in a variable or handle as needed
            }
        } else if (funcName == "Sleep") {
            if (!args.empty()) {
                int seconds = std::stoi(args[0]);
                std::this_thread::sleep_for(std::chrono::seconds(seconds));
            }
        } else if (functions.find(funcName) != functions.end()) {
            // Execute user-defined function
            std::cout << "Executing function: " << funcName << std::endl;
        }
    }
    
    DataType getDataType(const std::string& type) {
        if (type == "int") return INT_TYPE;
        if (type == "float") return FLOAT_TYPE;
        if (type == "bool") return BOOL_TYPE;
        if (type == "string") return STRING_TYPE;
        return VOID_TYPE;
    }
    
    // Expression evaluation
    bool evaluateCondition() {
        if (currentToken >= tokens.size()) return false;
        
        std::string left = parseExpression();
        if (currentToken >= tokens.size()) return false;
        
        std::string op = "";
        if (tokens[currentToken].type == OPERATOR) {
            op = tokens[currentToken].value;
            currentToken++;
        }
        
        if (op.empty()) {
            // Single value condition
            return evaluateBooleanValue(left);
        }
        
        std::string right = parseExpression();
        return evaluateComparison(left, op, right);
    }
    
    std::string parseExpression() {
        if (currentToken >= tokens.size()) return "";
        
        if (tokens[currentToken].type == STRING) {
            std::string value = tokens[currentToken].value;
            currentToken++;
            return value;
        } else if (tokens[currentToken].type == NUMBER) {
            std::string value = tokens[currentToken].value;
            currentToken++;
            return value;
        } else if (tokens[currentToken].type == IDENTIFIER) {
            std::string varName = tokens[currentToken].value;
            currentToken++;
            
            if (variables.find(varName) != variables.end()) {
                return variables[varName].value;
            }
            return varName;
        } else if (tokens[currentToken].type == KEYWORD && 
                   (tokens[currentToken].value == "true" || tokens[currentToken].value == "false")) {
            std::string value = tokens[currentToken].value;
            currentToken++;
            return value;
        }
        
        return "";
    }
    
    bool evaluateBooleanValue(const std::string& value) {
        if (value == "true") return true;
        if (value == "false") return false;
        if (value == "0") return false;
        if (value.empty()) return false;
        return true; // Non-zero numbers and non-empty strings are true
    }
    
    bool evaluateComparison(const std::string& left, const std::string& op, const std::string& right) {
        if (op == "==") {
            return left == right;
        } else if (op == "!=") {
            return left != right;
        } else if (op == "<") {
            return std::stod(left) < std::stod(right);
        } else if (op == ">") {
            return std::stod(left) > std::stod(right);
        } else if (op == "<=") {
            return std::stod(left) <= std::stod(right);
        } else if (op == ">=") {
            return std::stod(left) >= std::stod(right);
        }
        return false;
    }
    
    void parseIfStatement() {
        currentToken++; // Skip 'if'
        
        if (currentToken < tokens.size() && tokens[currentToken].value == "(") {
            currentToken++; // Skip '('
            
            bool condition = evaluateCondition();
            bool executed = false;
            
            if (currentToken < tokens.size() && tokens[currentToken].value == ")") {
                currentToken++; // Skip ')'
                
                if (currentToken < tokens.size() && tokens[currentToken].value == "{") {
                    currentToken++; // Skip '{'
                    
                    if (condition) {
                        // Execute if block
                        parseBlock();
                        executed = true;
                    } else {
                        // Skip if block
                        skipBlock();
                    }
                    
                    // Check for else if or else
                    while (currentToken < tokens.size() && tokens[currentToken].type == KEYWORD && tokens[currentToken].value == "else") {
                        currentToken++; // Skip 'else'
                        
                        if (currentToken < tokens.size() && tokens[currentToken].value == "if") {
                            // else if
                            currentToken++; // Skip 'if'
                            
                            if (currentToken < tokens.size() && tokens[currentToken].value == "(") {
                                currentToken++; // Skip '('
                                
                                bool elseIfCondition = evaluateCondition();
                                
                                if (currentToken < tokens.size() && tokens[currentToken].value == ")") {
                                    currentToken++; // Skip ')'
                                    
                                    if (currentToken < tokens.size() && tokens[currentToken].value == "{") {
                                        currentToken++; // Skip '{'
                                        
                                        if (!executed && elseIfCondition) {
                                            parseBlock();
                                            executed = true;
                                        } else {
                                            skipBlock();
                                        }
                                    }
                                }
                            }
                        } else if (currentToken < tokens.size() && tokens[currentToken].value == "{") {
                            // else block
                            currentToken++; // Skip '{'
                            
                            if (!executed) {
                                parseBlock();
                            } else {
                                skipBlock();
                            }
                            break; // else is the final block
                        }
                    }
                }
            }
        }
    }
    
    void parseBlock() {
        int braceCount = 1;
        while (currentToken < tokens.size() && braceCount > 0) {
            if (tokens[currentToken].value == "{") braceCount++;
            else if (tokens[currentToken].value == "}") braceCount--;
            
            if (braceCount > 0) {
                // Execute statement
                if (tokens[currentToken].type == KEYWORD) {
                    if (tokens[currentToken].value == "int" || 
                        tokens[currentToken].value == "float" || 
                        tokens[currentToken].value == "bool" || 
                        tokens[currentToken].value == "string") {
                        parseVariableDeclaration();
                    } else if (tokens[currentToken].value == "void") {
                        parseFunctionDeclaration();
                    } else if (tokens[currentToken].value == "if") {
                        parseIfStatement();
                    } else {
                        currentToken++;
                    }
                } else if (tokens[currentToken].type == IDENTIFIER) {
                    parseStatement();
                } else {
                    currentToken++;
                }
            }
        }
        if (currentToken < tokens.size() && tokens[currentToken].value == "}") {
            currentToken++; // Skip closing brace
        }
    }
    
    void skipBlock() {
        int braceCount = 1;
        while (currentToken < tokens.size() && braceCount > 0) {
            if (tokens[currentToken].value == "{") braceCount++;
            else if (tokens[currentToken].value == "}") braceCount--;
            currentToken++;
        }
    }
    
    void runFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return;
        }
        
        std::string code((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
        file.close();
        
        currentFile = filename;
        std::vector<Token> tokens = tokenize(code);
        parseAndExecute(tokens);
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: crimson_interpreter <filename.crm>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    if (filename.substr(filename.find_last_of(".") + 1) != "crm") {
        std::cout << "Error: File must have .crm extension" << std::endl;
        return 1;
    }
    
    CrimsonInterpreter interpreter;
    interpreter.runFile(filename);
    
    return 0;
}
