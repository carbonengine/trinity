// Copyright © 2012 CCP ehf.

#include "StdAfx.h"

#ifndef _MSC_VER
#define _wcsnicmp wcsncasecmp
#endif

enum States
{
	STATE_ROOT,
	STATE_GENERAL_TEXT,
	STATE_TAG,
	STATE_CLOSE_TAG,

	// Detection of &amp; and the like
	STATE_GT_AMPSTART,

	STATE_TERMINATE
};

#if BLUE_WITH_PYTHON

namespace
{
PyObject* ToPyUnicode( const wchar_t* str, Py_ssize_t size )
{
#if PY_MAJOR_VERSION == 2
	return PyUnicode_FromUnicode( reinterpret_cast<const Py_UNICODE*>( str ), size );
#else
	return PyUnicode_FromWideChar( str, size );
#endif
}
PyObject* ToPyUnicode( const wchar_t* str )
{
#if PY_MAJOR_VERSION == 2
	return PyUnicode_FromUnicode( reinterpret_cast<const Py_UNICODE*>( str ), wcslen( str ) );
#else
	return PyUnicode_FromWideChar( str, -1 );
#endif
}
}

// Returns nullptr if the general attribute pattern of ((whitespace)*attrib=value)*> isn't matched
// Returns Py_None if you're pointing at > already (no attributes)
// Returns a dict of attributes on success.
//
PyObject* ParseAttribs( wchar_t*& curPos, const wchar_t* keyAlreadyParsed = NULL )
{
	if( *curPos == L'>' )
	{
		// Well, that was easy, there aren't any.
		curPos++;
		Py_INCREF( Py_None );
		return Py_None;
	}

	enum attribStates
	{
		ASTATE_KEY,
		ASTATE_VALUE,
		ASTATE_WHITEOREND,
		ASTATE_WHITESPACE,
		ASTATE_TERMINATE
	};

	attribStates astate;
	const wchar_t* key = nullptr;
	PyObject* result = PyDict_New();

	if( keyAlreadyParsed )
	{
		key = keyAlreadyParsed;
		astate = ASTATE_VALUE;
	}
	else
	{
		astate = ASTATE_WHITESPACE;
	}

	while( astate != ASTATE_TERMINATE )
	{
		switch( astate )
		{
		case ASTATE_WHITEOREND:
			if( *curPos == L'>' )
			{
				curPos++;
				astate = ASTATE_TERMINATE;
				break;
			}
			// Fall through
		case ASTATE_WHITESPACE: {
			bool matched = false;
			bool wasWhite = true;

			while( wasWhite )
			{
				wchar_t next = *curPos;
				if( next == L' ' || next == L'\t' )
				{
					curPos++;
					matched = true;
				}
				else if( next == L'&' && _wcsnicmp( curPos + 1, L"nbsp;", 5 ) == 0 )
				{
					curPos += 6;
					matched = true;
				}
				else
				{
					wasWhite = false;
				}
			}

			if( !matched )
			{
				// There wasn't any whitespace where there should be.  We're hosed.
				goto error;
			}
			else
			{
				astate = ASTATE_KEY;
			}
		}
		break;
		case ASTATE_KEY: {
			key = curPos;
			// March along until we find =
			//  - or error cases like > and EOF
			//  - or odd but handleable cases like whitespace

			wchar_t curChar = *curPos;
			while( curChar != L'=' )
			{
				if( curChar == L'>' )
				{
					goto error;
				}
				if( curChar == L'\0' )
				{
					goto error;
				}

				if( curChar == L' ' || curChar == L'\t' )
				{
					// We just passed over a word, but whatever.
					key = curPos + 1;
				}

				if( curChar == L'&' && _wcsnicmp( curPos + 1, L"nbsp;", 5 ) == 0 )
				{
					key = curPos + 6;
					curPos += 5; // Only 5 'cause we're about to do another ++.
				}

				curPos++;
				curChar = *curPos;
			}

			// We made it without error - we're now pointing at an =, which we're going to turn into a null
			// so we can use key as a null-term string.
			*curPos = '\0';
			curPos++;
			astate = ASTATE_VALUE;
		}
		break;
		case ASTATE_VALUE: {
			PyObject* value = nullptr;
			if( *curPos == L'\'' || *curPos == L'"' )
			{
				// Sweet, it's a quoted string, we just zoom to the end of the quote and that's that.
				wchar_t* endOfValue = wcschr( curPos + 1, *curPos );
				if( !endOfValue )
				{
					goto error;
				}
				// Get string value without begin/end quotes then advance the current position after the end quote.
				value = ToPyUnicode( curPos + 1, endOfValue - curPos - 1 );
				curPos = endOfValue + 1;
			}
			else
			{
				// Not a quoted string...go until we find > or whitespace.
				wchar_t* startOfValue = curPos;
				bool done = false;
				while( !done )
				{
					wchar_t curChar = *curPos;

					if( curChar == L' ' || curChar == L'\t' || curChar == L'>' ||
						( curChar == L'&' && _wcsnicmp( curPos + 1, L"nbsp;", 5 ) == 0 ) )
					{
						done = true;
					}
					else if( curChar == L'\0' )
					{
						goto error;
					}
					else
					{
						curPos++;
					}
				}

				value = ToPyUnicode( startOfValue, curPos - startOfValue );
			}

			PyObject* keyThunked = ToPyUnicode( key );
			PyDict_SetItem( result, keyThunked, value );

			astate = ASTATE_WHITEOREND;
		}
		break;
		default:
			break;
		}
	}


	return result;

error:
	// We've hit an unknown spot.  Try to fast forward to an > and barring that, the end of the string.
	wchar_t* closeTag = wcschr( curPos, L'>' );
	if( closeTag )
	{
		curPos = closeTag + 1;
	}
	else
	{
		//curPos = wcschr(curPos, L'\0');
		// Sure, we could zoom forward to the natural NULL, but since we can slice
		// and dice up the input as we like, let's just put one where we are.
		*curPos = '\0';
	}
	Py_DECREF( result ); // He isn't going anywhere, so toss him out.
	return NULL;
}

// A little helper function (well, define, but it's a minor difference) to consume whatever
// text we've accumulated up to this point.
//
#define CONSUME_TEXT                                                                                \
	{ /* Consume the text up to this point into a text-element */                                   \
		PyObject* textTuple = PyTuple_New( 2 );                                                     \
		PyTuple_SetItem( textTuple, 0, ToPython( 0 ) );                                             \
		PyObject* newString;                                                                        \
                                                                                                    \
		if( stringBeingBuilt.empty() )                                                              \
		{                                                                                           \
			/* Simple case, we haven't done any substitutions. */                                   \
			newString = ToPyUnicode( marker, ( curPos - marker ) );                                 \
		}                                                                                           \
		else                                                                                        \
		{                                                                                           \
			/* We did some string manip, so finish it off then make a new thunked-string for it. */ \
			if( curPos != marker )                                                                  \
			{                                                                                       \
				stringBeingBuilt.append( marker, curPos - marker );                                 \
			}                                                                                       \
			newString = ToPyUnicode( stringBeingBuilt.c_str(), stringBeingBuilt.size() );           \
			stringBeingBuilt.clear();                                                               \
		}                                                                                           \
                                                                                                    \
		PyTuple_SetItem( textTuple, 1, newString );                                                 \
		PyList_Append( currentTab, textTuple );                                                     \
		Py_DECREF( textTuple );                                                                     \
	}

static PyObject* PyParseLabelText( PyObject* self, PyObject* args )
{
#if PY_MAJOR_VERSION == 2
	wchar_t* paramString;
	if( !PyArg_ParseTuple( args, "u", &paramString ) )
	{
		return nullptr;
	}

	size_t stringLength = wcslen( paramString );
#else
	PyObject* paramObject;
	if( !PyArg_ParseTuple( args, "U", &paramObject ) )
	{
		return nullptr;
	}
	Py_ssize_t stringLength;
	wchar_t* paramString = PyUnicode_AsWideCharString( paramObject, &stringLength );
	if( !paramString )
	{
		return nullptr;
	}
#endif
	wchar_t* inString = CCP_NEW( "ParseLabelText" ) wchar_t[stringLength + 1];
	wcscpy_s( inString, stringLength + 1, paramString );

	PyObject* listOfLines = PyList_New( 0 );

	PyObject* currentLine = PyList_New( 0 );
	PyObject* currentTab = PyList_New( 0 );
	PyList_Append( listOfLines, currentLine );
	PyList_Append( currentLine, currentTab );

	// The list now owns the references
	Py_DECREF( currentLine );
	Py_DECREF( currentTab );

	wchar_t* curPos = inString;
	wchar_t* marker = curPos;

	std::wstring stringBeingBuilt; // Can't just copy straight over 'cause we sometimes do text replacements, like &amp;

	States state = STATE_ROOT;

	while( state != STATE_TERMINATE )
	{
		switch( state )
		{
		case STATE_ROOT: // Very simple state for starting out, we're either at a tag or we aren't.
			if( *curPos == L'<' )
			{
				marker = curPos;
				curPos++;
				state = STATE_TAG;
			}
			else if( *curPos == L'\0' )
			{
				state = STATE_TERMINATE;
			}
			else
			{
				marker = curPos;
				state = STATE_GENERAL_TEXT;
			}
			break;

		case STATE_GENERAL_TEXT:
			switch( *curPos )
			{
			case L'<':
				CONSUME_TEXT;
				curPos++;
				state = STATE_TAG;
				break;

			case L'&':
				state = STATE_GT_AMPSTART;
				break;

			case L'\t':
				*curPos = L' '; // Single character replacements are easy!
				curPos++;
				break;

			case L'\0':
				CONSUME_TEXT;
				state = STATE_TERMINATE;
				break;

			case L'\n':
				CONSUME_TEXT;
				curPos++;

				// Start up a new line
				currentLine = PyList_New( 0 );
				currentTab = PyList_New( 0 );
				PyList_Append( listOfLines, currentLine );
				PyList_Append( currentLine, currentTab );

				// The list now owns the references
				Py_DECREF( currentLine );
				Py_DECREF( currentTab );

				state = STATE_ROOT;
				break;

			case L'\r':
				if( *( curPos + 1 ) == L'\n' )
				{
					CONSUME_TEXT;
					curPos += 2;

					// Start up a new line
					currentLine = PyList_New( 0 );
					currentTab = PyList_New( 0 );
					PyList_Append( listOfLines, currentLine );
					PyList_Append( currentLine, currentTab );

					// The list now owns the references
					Py_DECREF( currentLine );
					Py_DECREF( currentTab );

					state = STATE_ROOT;
				}
				else
				{
					curPos++;
				}
				break;

			default:
				// Just advance on past, we'll copy the text from marker to curPos when we need to.
				curPos++;
				break;
			}

			break;
		case STATE_TAG: {
			// This one's going to be a bit complex...
			// We're pointing at right after the open <

			if( *curPos == L'/' )
			{
				// Whoops, we're in a close tag, not an open tag!
				curPos++;
				state = STATE_CLOSE_TAG;
				break;
			}

			bool matched = false;
			wchar_t next;

			switch( *curPos )
			{
			case L'a': // <a ...>
			case L'A': {
				// Matched <a ...>, assuming we match attributes...
				curPos++;
				PyObject* attribs = ParseAttribs( curPos );
				if( attribs )
				{
					matched = true;

					PyObject* tagTuple = PyTuple_New( 3 );
					PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
					PyTuple_SetItem( tagTuple, 1, ToPython( 6 ) );
					PyTuple_SetItem( tagTuple, 2, attribs );
					PyList_Append( currentTab, tagTuple );
					Py_DECREF( tagTuple );
				}
			}
			break;

			case L'b': // <br> (and all forms) and <b>
			case L'B':
				next = *( curPos + 1 );
				if( next == L'r' || next == L'R' )
				{
					// We can now eat whitespace if we want.
					wchar_t* wsEater = curPos + 2;

					bool wsExhausted = false;
					while( !wsExhausted )
					{
						next = *wsEater;
						if( next == L' ' || next == L'\t' )
						{
							wsEater++;
							continue;
						}

						// Is it our good friend, &nbsp;?
						if( next == L'&' )
						{
							if( _wcsnicmp( wsEater + 1, L"nbsp;", 5 ) == 0 )
							{
								wsEater += 6; // omnomnom
								continue;
							}
						}

						// Well, no matches, let's term.
						wsExhausted = true;
					}

					// We now match either "/>" or ">"
					next = *( wsEater );
					if( next == L'>' )
					{
						curPos = wsEater + 1;
						matched = true;
					}
					else if( next == L'/' && *( wsEater + 1 ) == L'>' )
					{
						curPos = wsEater + 2;
						matched = true;
					}

					if( matched )
					{
						// Start up a new line
						currentLine = PyList_New( 0 );
						currentTab = PyList_New( 0 );
						PyList_Append( listOfLines, currentLine );
						PyList_Append( currentLine, currentTab );

						// The list now owns the references
						Py_DECREF( currentLine );
						Py_DECREF( currentTab );
					}
				}
				else if( next == L'>' )
				{
					// Matched <b>
					matched = true;
					PyObject* tagTuple = PyTuple_New( 3 );
					PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
					PyTuple_SetItem( tagTuple, 1, ToPython( 5 ) );
					Py_INCREF( Py_None );
					PyTuple_SetItem( tagTuple, 2, Py_None );
					PyList_Append( currentTab, tagTuple );
					Py_DECREF( tagTuple );
					curPos += 2;
				}
				break;

			case L'c': // <center>, <color= attribtag
			case L'C':
				next = *( curPos + 1 );
				if( next == L'e' || next == L'E' )
				{
					if( _wcsnicmp( curPos + 2, L"nter>", 5 ) == 0 )
					{
						// Matched <center>
						matched = true;
						PyObject* tagTuple = PyTuple_New( 3 );
						PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
						PyTuple_SetItem( tagTuple, 1, ToPython( 102 ) );
						Py_INCREF( Py_None );
						PyTuple_SetItem( tagTuple, 2, Py_None );
						PyList_Append( currentTab, tagTuple );
						Py_DECREF( tagTuple );
						curPos += 7;
					}
				}
				else if( next == L'o' || next == L'O' )
				{
					if( _wcsnicmp( curPos + 2, L"lor=", 4 ) == 0 )
					{
						// We have ourselves an attrib-tag!  See if we parse attribs...
						curPos += 6;
						PyObject* attribs = ParseAttribs( curPos, L"color" );
						if( attribs )
						{
							matched = true;
							PyObject* tagTuple = PyTuple_New( 3 );
							PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
							PyTuple_SetItem( tagTuple, 1, ToPython( -0xBEEF ) );
							PyTuple_SetItem( tagTuple, 2, attribs );
							PyList_Append( currentTab, tagTuple );
							Py_DECREF( tagTuple );
						}
					}
				}
				break;

			case L'f': // <font ...>, <fontsize= attribtag
			case L'F':
				if( _wcsnicmp( curPos + 1, L"ont", 3 ) == 0 )
				{
					next = *( curPos + 4 );
					if( next == L's' || next == L'S' )
					{
						if( _wcsnicmp( curPos + 5, L"ize=", 4 ) == 0 )
						{
							// <fontsize= attribtag!
							curPos += 9;
							PyObject* attribs = ParseAttribs( curPos, L"fontsize" );
							if( attribs )
							{
								matched = true;
								PyObject* tagTuple = PyTuple_New( 3 );
								PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
								PyTuple_SetItem( tagTuple, 1, ToPython( -0xBEEF ) );
								PyTuple_SetItem( tagTuple, 2, attribs );
								PyList_Append( currentTab, tagTuple );
								Py_DECREF( tagTuple );
							}
						}
					}
					else
					{
						// Maybe it's <font ...>, try to match attribs.
						curPos += 4;
						PyObject* attribs = ParseAttribs( curPos );
						if( attribs )
						{
							matched = true;

							PyObject* tagTuple = PyTuple_New( 3 );
							PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
							PyTuple_SetItem( tagTuple, 1, ToPython( 1 ) );
							PyTuple_SetItem( tagTuple, 2, attribs );
							PyList_Append( currentTab, tagTuple );
							Py_DECREF( tagTuple );
						}
					}
				}
				break;

			case L'h': // <hint= attribtag
			case L'H':
				if( _wcsnicmp( curPos + 1, L"int=", 4 ) == 0 )
				{
					curPos += 5;
					PyObject* attribs = ParseAttribs( curPos, L"hint" );
					if( attribs )
					{
						matched = true;
						PyObject* tagTuple = PyTuple_New( 3 );
						PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
						PyTuple_SetItem( tagTuple, 1, ToPython( -0xBEEF ) );
						PyTuple_SetItem( tagTuple, 2, attribs );
						PyList_Append( currentTab, tagTuple );
						Py_DECREF( tagTuple );
					}
				}
				break;

			case L'i': // <i>
			case L'I':
				if( *( curPos + 1 ) == L'>' )
				{
					// Matched <i>
					matched = true;
					PyObject* tagTuple = PyTuple_New( 3 );
					PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
					PyTuple_SetItem( tagTuple, 1, ToPython( 4 ) );
					Py_INCREF( Py_None );
					PyTuple_SetItem( tagTuple, 2, Py_None );
					PyList_Append( currentTab, tagTuple );
					Py_DECREF( tagTuple );
					curPos += 2;
				}
				break;

			case L'l': // <left>, <localized>, <letterspace= attribtag
			case L'L':
				next = *( curPos + 1 );
				if( next == L'o' || next == L'O' )
				{
					if( _wcsnicmp( curPos + 2, L"calized", 7 ) == 0 )
					{
						// Matched <localized, see if we have attribs
						curPos += 9;
						PyObject* attribs = ParseAttribs( curPos );
						if( attribs )
						{
							matched = true;

							PyObject* tagTuple = PyTuple_New( 3 );
							PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
							PyTuple_SetItem( tagTuple, 1, ToPython( 7 ) );
							PyTuple_SetItem( tagTuple, 2, attribs );
							PyList_Append( currentTab, tagTuple );
							Py_DECREF( tagTuple );
						}
					}
				}
				else if( next == L'e' || next == L'E' )
				{
					if( _wcsnicmp( curPos + 2, L"ft>", 3 ) == 0 )
					{
						// Matched <left>
						matched = true;
						PyObject* tagTuple = PyTuple_New( 3 );
						PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
						PyTuple_SetItem( tagTuple, 1, ToPython( 100 ) );
						Py_INCREF( Py_None );
						PyTuple_SetItem( tagTuple, 2, Py_None );
						PyList_Append( currentTab, tagTuple );
						Py_DECREF( tagTuple );
						curPos += 5;
					}
					else if( _wcsnicmp( curPos + 2, L"tterspace=", 10 ) == 0 )
					{
						// Matching letterspace attribtag?
						curPos += 12;
						PyObject* attribs = ParseAttribs( curPos, L"letterspace" );
						if( attribs )
						{
							matched = true;
							PyObject* tagTuple = PyTuple_New( 3 );
							PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
							PyTuple_SetItem( tagTuple, 1, ToPython( -0xBEEF ) );
							PyTuple_SetItem( tagTuple, 2, attribs );
							PyList_Append( currentTab, tagTuple );
							Py_DECREF( tagTuple );
						}
					}
				}
				break;

			case L'r': // <right>
			case L'R':
				if( _wcsnicmp( curPos + 1, L"ight>", 5 ) == 0 )
				{
					// Matched <right>
					matched = true;
					PyObject* tagTuple = PyTuple_New( 3 );
					PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
					PyTuple_SetItem( tagTuple, 1, ToPython( 101 ) );
					Py_INCREF( Py_None );
					PyTuple_SetItem( tagTuple, 2, Py_None );
					PyList_Append( currentTab, tagTuple );
					Py_DECREF( tagTuple );
					curPos += 6;
				}
				break;

			case L't': // <t>
			case L'T':
				if( *( curPos + 1 ) == L'>' )
				{
					// Matched <t>
					matched = true;
					curPos += 2;

					// start up a new tab
					currentTab = PyList_New( 0 );
					PyList_Append( currentLine, currentTab );
					Py_DECREF( currentTab );
				}
				break;

			case L'u': // <u>, <uppercase>, <url: & <url= attribtags
			case L'U':
				next = *( curPos + 1 );
				if( next == L'>' )
				{
					// Matched <u>
					matched = true;
					PyObject* tagTuple = PyTuple_New( 3 );
					PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
					PyTuple_SetItem( tagTuple, 1, ToPython( 2 ) );
					Py_INCREF( Py_None );
					PyTuple_SetItem( tagTuple, 2, Py_None );
					PyList_Append( currentTab, tagTuple );
					Py_DECREF( tagTuple );
					curPos += 2;
				}
				else if( next == L'r' || next == L'R' )
				{
					next = *( curPos + 2 );
					if( next == L'l' || next == L'L' )
					{
						if( *( curPos + 3 ) == L':' )
						{
							// Kill this wicked syntax!
							curPos[3] = L'=';
						}
						if( *( curPos + 3 ) == L'=' )
						{
							// Possible match for url= attribtag (and url:, 'cause we just munged that)
							curPos += 4;
							PyObject* attribs = ParseAttribs( curPos, L"url" );
							if( attribs )
							{
								matched = true;
								PyObject* tagTuple = PyTuple_New( 3 );
								PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
								PyTuple_SetItem( tagTuple, 1, ToPython( -0xBEEF ) );
								PyTuple_SetItem( tagTuple, 2, attribs );
								PyList_Append( currentTab, tagTuple );
								Py_DECREF( tagTuple );
							}
						}
					}
				}
				else if( next == L'p' || next == L'P' )
				{
					if( _wcsnicmp( curPos + 2, L"percase>", 8 ) == 0 )
					{
						// Matched <uppercase>
						matched = true;
						PyObject* tagTuple = PyTuple_New( 3 );
						PyTuple_SetItem( tagTuple, 0, ToPython( 1 ) );
						PyTuple_SetItem( tagTuple, 1, ToPython( 3 ) );
						Py_INCREF( Py_None );
						PyTuple_SetItem( tagTuple, 2, Py_None );
						PyList_Append( currentTab, tagTuple );
						Py_DECREF( tagTuple );
						curPos += 10;
					}
				}

				break;
			}

			if( !matched )
			{
				wchar_t* closeTag = wcschr( curPos, L'>' );
				if( closeTag )
				{
					PyObject* unknownTag = PyTuple_New( 2 );
					PyTuple_SetItem( unknownTag, 0, ToPython( 3 ) );
					PyTuple_SetItem( unknownTag, 1, ToPyUnicode( curPos, closeTag - curPos ) );
					PyList_Append( currentTab, unknownTag );
					Py_DECREF( unknownTag );

					curPos = closeTag + 1;

					state = STATE_ROOT; // Don't know if we're rockin' text or a tag or whatnot next.
				}
				else
				{
					// Mismatched <>, TODO: report this somehow.
					state = STATE_TERMINATE;
				}
			}
			else
			{
				state = STATE_ROOT; // Don't know if we're rockin' text or a tag or whatnot next.
			}
		}
		break;
		case STATE_CLOSE_TAG: {
			int closeTagID = -1;
			switch( *curPos )
			{
			case L'a':
			case L'A':
				if( *( curPos + 1 ) == L'>' )
				{
					closeTagID = 6;
					curPos += 2;
				}
				break;
			case L'b':
			case L'B':
				if( *( curPos + 1 ) == L'>' )
				{
					closeTagID = 5;
					curPos += 2;
				}
				break;
			case L'c':
			case L'C':
				if( _wcsnicmp( curPos + 1, L"olor>", 5 ) == 0 )
				{
					closeTagID = 200;
					curPos += 6;
				}
				break;
			case L'f':
			case L'F':
				if( _wcsnicmp( curPos + 1, L"ont", 3 ) == 0 )
				{
					if( *( curPos + 4 ) == L'>' )
					{
						closeTagID = 1;
						curPos += 5;
					}
					else if( _wcsnicmp( curPos + 4, L"size>", 5 ) == 0 )
					{
						closeTagID = 201;
						curPos += 9;
					}
				}
				break;
			case L'h':
			case L'H':
				if( _wcsnicmp( curPos + 1, L"int>", 4 ) == 0 )
				{
					closeTagID = 203;
					curPos += 5;
				}
				break;
			case L'i':
			case L'I':
				if( *( curPos + 1 ) == L'>' )
				{
					closeTagID = 4;
					curPos += 2;
				}
				break;
			case L'l':
			case L'L':
				if( _wcsnicmp( curPos + 1, L"etterspace>", 11 ) == 0 )
				{
					closeTagID = 202;
					curPos += 12;
				}
				else if( _wcsnicmp( curPos + 1, L"ocalized>", 9 ) == 0 )
				{
					closeTagID = 7;
					curPos += 10;
				}
				break;
			case L'u':
			case L'U':
				if( *( curPos + 1 ) == L'>' )
				{
					closeTagID = 2;
					curPos += 2;
				}
				else if( _wcsnicmp( curPos + 1, L"rl>", 3 ) == 0 )
				{
					closeTagID = 204;
					curPos += 4;
				}
				else if( _wcsnicmp( curPos + 1, L"ppercase>", 9 ) == 0 )
				{
					closeTagID = 3;
					curPos += 10;
				}
				break;
			}

			if( closeTagID != -1 )
			{
				PyObject* closeTuple = PyTuple_New( 2 );
				PyTuple_SetItem( closeTuple, 0, ToPython( 2 ) );
				PyTuple_SetItem( closeTuple, 1, ToPython( closeTagID ) );
				PyList_Append( currentTab, closeTuple );
				Py_DECREF( closeTuple );
			}
			else
			{
				// We didn't match a close tag...just march on to > and carry on.
				wchar_t* closeTag = wcschr( curPos, L'>' );
				if( closeTag )
				{
					curPos = closeTag + 1;
				}
				else
				{
					//curPos = wcschr(curPos, L'\0');
					// Sure, we could zoom forward to the natural NULL, but since we can slice
					// and dice up the input as we like, let's just put one where we are.
					*curPos = '\0';
				}
			}

			state = STATE_ROOT; // What's next?  Who knows!
		}
		break;
		case STATE_GT_AMPSTART:
			// Inside of a general text, we have an amp.  Let's see if we need to do anything with it.
			// Note, we're currently pointing at the amp.
			{
				bool matched = false;
				wchar_t next;
				switch( *( curPos + 1 ) )
				{
				case L'a':
				case L'A':
					if( _wcsnicmp( curPos + 2, L"mp;", 3 ) == 0 )
					{
						// We matched &amp;
						// Copy up from the marker to here into the building-string
						// then emit an & and off we go.
						if( marker != curPos )
						{
							stringBeingBuilt.append( marker, curPos - marker );
						}
						stringBeingBuilt.append( (size_t)1, L'&' );
						curPos += 5;
						marker = curPos;
						matched = true;
					}
					break;
				case L'l':
				case L'L':
					next = *( curPos + 2 );
					if( next == L't' || next == L'T' )
					{
						next = *( curPos + 3 );
						if( next == L';' )
						{
							// We matched &lt;
							// Copy up from the marker to here into the building-string
							// then emit an & and off we go.
							if( marker != curPos )
							{
								stringBeingBuilt.append( marker, curPos - marker );
							}
							stringBeingBuilt.append( 1, L'<' );
							curPos += 4;
							marker = curPos;
							matched = true;
						}
					}
					break;
				case L'g':
				case L'G':
					next = *( curPos + 2 );
					if( next == L't' || next == L'T' )
					{
						next = *( curPos + 3 );
						if( next == L';' )
						{
							// We matched &gt;
							// Copy up from the marker to here into the building-string
							// then emit an & and off we go.
							if( marker != curPos )
							{
								stringBeingBuilt.append( marker, curPos - marker );
							}
							stringBeingBuilt.append( 1, L'>' );
							curPos += 4;
							marker = curPos;
							matched = true;
						}
					}
					break;
				case L'n':
				case L'N':
					if( _wcsnicmp( curPos + 2, L"bsp;", 4 ) == 0 )
					{
						// We matched &nbsp;
						// Copy up from the marker to here into the building-string
						// then emit an & and off we go.
						if( marker != curPos )
						{
							stringBeingBuilt.append( marker, curPos - marker );
						}
						stringBeingBuilt.append( 1, L' ' );
						curPos += 6;
						marker = curPos;
						matched = true;
					}
					break;
				default:
					break;
				}
				if( !matched )
				{
					// Well, didn't match any of our patterns, just carry on like nothing happened.
					curPos++;
				}
				// In any case, back to gen text with us.
				state = STATE_GENERAL_TEXT;
			}
			break;
		default:
			break;
		}
	}

	CCP_DELETE[] inString;
	return listOfLines;
}

MAP_FUNCTION(
	"ParseLabelText",
	PyParseLabelText,
	"This is used by the UI's Label to parse our custom HTML-ish language very quickly.  It is almost surely not useful for any other purpose.\n"
	":param text: input text\n"
	":type text: unicode\n"
	":rtype: list" );

#endif
