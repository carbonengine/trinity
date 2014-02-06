#pragma once

#include "InlineString.h"

class SymbolTable;
class ASTNode;
struct Symbol;

struct FileLocation
{
	unsigned lineNumber;
	InlineString fileName;
};

struct ScannerToken
{
	int type;
	long intValue;
	InlineString stringValue;
	FileLocation fileLocation;
};

enum PreprocessorScanResult
{
	PPSR_ERROR,
	PPSR_EOF,
	PPSR_OK,
};

enum PreprocessorTokenType
{
	PPT_ID,
	PPT_INT_CONST,
	PPT_FLOAT_CONST,
	PPT_CHAR_CONST,
	PPT_STRING_CONST,
	PPT_OPERATOR,
	PPT_PP_OPERATOR,
};

struct PreprocessorToken
{
	PreprocessorTokenType type;
	InlineString string;
	FileLocation fileLocation;
};

struct PreprocessorDefine
{
	FileLocation location;
	std::vector<InlineString> parameters;
	InlineString value;
};

enum ErrorCode
{
	// params: %s - token string
	EC_SYNTAX_ERROR,
	// params: none
	EC_UNEXPECTED_EOF,
	// params: %c - character
	EC_UNEXPECTED_CHARACTER,
	// params: %s - identifier name
	EC_UNDECLARED_IDENTIFIER,
	// params: %s - identifier name
	EC_IDENTIFIER_REDEFINITION,
	// params: %s - identifier name
	EC_PARAMETER_REDEFINITION,
	// params: none
	EC_INVALID_PACK_OFFSET,
	// params: none
	EC_INVALID_REGISTER,
	// params: none
	EC_NO_TECHNIQUE,
	// params: %s - annotation name
	EC_ANNOTATION_TYPE_MISMATCH,
	// params: none
	EC_REGISTER_ASSIGNMENT_DEPRECATED,
	// params: %s - function name
	EC_NO_OVERRIDE,


	// params: none
	EC_INVALID_INDEXING,
	// params: %s - substript
	EC_INVALID_SUBSCRIPT,
	// params: none
	EC_TYPE_MISMATCH,
	// params: none
	EC_INT_TYPE_REQUIRED,
	// params: none
	EC_SCALAR_VECTOR_TYPE_REQUIRED,
	// params: %s - from type, %s - to type
	EC_INVALID_CAST,

	// FX state evaluation
	// params: %s - state name
	EC_INVALID_STATE,
	// params: %s - state value
	EC_INVALID_STATE_VALUE,
	// params: %s - type name
	EC_UNSUPPORTED_TYPE,
	// params: %s - symbol name
	EC_UNINITIALIZED_VAR,
	// params: none
	EC_STRUCTS_NOT_SUPPORTED,
	// params: none
	EC_INVALID_IMPLICIT_CAST,
	// params: %s - operator name
	EC_OPERATOR_TYPE_MISMATCH,
	// params: %s - operator name
	EC_SIDE_EFFECT_IN_STATE_ASSIGMENT,
	// params: %i - index value
	EC_INDEX_OUT_OF_RANGE,
	// params: %s - swizzle
	EC_INVALID_SWIZZLE,
	// params: %s - function name
	EC_FUNCTIONS_NOT_SUPPORTED,
	// params: none
	EC_INCORRECT_NUMBER_OF_ARGS,

	// params: %s - sampler name, %s - type, %s - type
	EC_MISMATCHED_SAMPLER_TYPE,
	// params: %s - sampler name, %s - type, %s - type
	EC_MISMATCHED_TEXTURE_TYPE,
	// params: %s - sampler name
	EC_SAMPLER_WITHOUT_TEXTURE,

	// params: %s - symbol name
	EC_PREPROCESSOR_UNDEFINED_SYMBOL,
	// params: none
	EC_PREPROCESSOR_MISPLACED_ELIF,
	// params: none
	EC_PREPROCESSOR_MISPLACED_ELSE,
	// params: none
	EC_PREPROCESSOR_MISPLACED_ENDIF,
	// params: %s - path
	EC_FILE_NOT_FOUND,
	// params: none
	EC_EOF_IN_MACRO,
	// params: %s - macro name
	EC_INCORRECT_MACRO_PARAMETER_COUNT,
	// params: none
	EC_INVALID_INCLUDE_PATH,
	// params: none
	EC_UNEXPECTED_EOL,
	// params: %s - user message
	EC_USER_ERROR,
	// params: %s - state name
	EC_STATE_DEPRECATED,
	// params: none
	EC_MULTIPLE_REGISTER_BINDINGS,
};

struct PreprocessorCondition
{
	bool skipMode;
	bool inheridedSkipMode;
	bool used;
	bool seenElse;
};

class ParserState
{
public:
	enum Mode
	{
		HLSL,
		FX,
	};

	ParserState( const InlineString& code );
	~ParserState();

	bool Parse();
	PreprocessorScanResult EvaluatePreprocessorCondition( const InlineString& string, bool& result );

	bool HasErrors() const;
	ASTNode* GetTree();
	SymbolTable& GetSymbolTable();

	FileLocation& GetCurrentLocation();

	InlineString AllocateName();
	char* AllocateString( size_t size );

	void ShowMessage( const FileLocation& location, ErrorCode errorCode, ... );
	void ShowMessage( const ScannerToken& token, ErrorCode errorCode, ... );
	void ShowMessage( ErrorCode errorCode, ... );

	void AddOfflineStatement( ASTNode* node );
	void MoveOfflineStatements( ASTNode* to );
	void SetTree( ASTNode* root );

	const char*& GetStreamPosition();
	const char* GetStreamEnd();
	bool InMacro() const;
	bool InSkipMode() const;
	PreprocessorScanResult GetPreprocessorToken( PreprocessorToken& token );

	void IncludeFile( const InlineString& fileName );
	void ParseDefine( const PreprocessorDefine& define, const FileLocation& location, const std::vector<InlineString>& arguments );
	PreprocessorDefine* FindDefine( const InlineString& name );

	void AddPragma( const InlineString& string );
	void ResetPragmaUsage();
	bool GetPragma( const FileLocation& location, InlineString& string );

	const std::set<Symbol*>& GetDX9TextureFunctions();
	const std::map<const char*, Symbol*>& GetDX9Functions();

	bool m_newLine;
	Mode m_mode;
	int m_preprocessorConditionResult;
	bool m_expandMacros;

	std::map<InlineString, PreprocessorDefine> m_defines;
	std::vector<PreprocessorCondition> m_prepocessorConditions;
private:
	void ShowMessageImpl( const FileLocation& location, ErrorCode errorCode, va_list args );
	bool FindConcatOperator( size_t depth );
	bool ParseMacroArguments( std::vector<InlineString>& arguments );
	void AddIntrinsics();

	struct FileContents
	{
		FileLocation location;
		InlineString code;
		const char* position;
		bool isMacro;
		std::map<InlineString, PreprocessorDefine> arguments;
	};
	std::vector<FileContents> m_fileStack;

	struct Pragma
	{
		InlineString pragma;
		FileLocation location;
		bool used;
	};
	std::vector<Pragma> m_pragmas;

	SymbolTable* m_symbols;
	ASTNode* m_root;
	ASTNode* m_offlineStatements;
	bool m_hasErrors;
	bool m_inPreprocessorCondition;

	std::vector<char*> m_strings;
	std::set<Symbol*> m_dx9TextureFunctions;
	std::map<const char*, Symbol*> m_dx9Functions;
};
