#include "AlifLexer.h"

#include <QString>


// QVector<Token> Lexer::tokenize(const QString& text) {
// 	while (pos < text.length()) {
// 		QChar currentChar = text[pos];

// 		if (currentChar.isSpace()) {
// 			while (pos < text.length() and text[pos].isSpace()) {
// 				pos++;
// 			}
// 		}
// 		else if (currentChar.isDigit()) {
// 			int start = pos;
// 			while (pos < text.length() and (text[pos].isDigit() or text[pos] == '.')) {
// 				pos++;
// 			}
// 			tokens.append(Token(TokenType::Number, start, pos - start, text.mid(start, pos - start)));
// 		}
// 		else if (currentChar.isLetter() or currentChar == '_') {
// 			int start = pos;
// 			while (pos < text.length() and (text[pos].isLetterOrNumber() or text[pos] == '_')) {
// 				pos++;
// 			}
// 			QString identifier = text.mid(start, pos - start);
// 			TokenType type = isKeyword(identifier) ? TokenType::Keyword : isKeyword1(identifier) ? TokenType::Keyword1 : isKeyword2(identifier) ? TokenType::Keyword2 : TokenType::Identifier;
// 			tokens.append(Token(type, start, pos - start, identifier));
// 		}
// 		else if (currentChar == '"' or currentChar == '\'') {
// 			int stringStart = pos;
// 			quoteCount++;
// 			pos++; // Move past the opening quote

// 			// Check if the string is an f-string (starts with u'م')
// 			isFString += (stringStart > 0 and text[stringStart - 1] == u'م');

// 			while (pos < text.length()) {
// 				if (text[pos] == '\\') {
// 					// Handle escape sequences
// 					pos += 2; // Skip the escape character
// 				}
// 				else if (text[pos] == currentChar) {
// 					// Found the closing quote
// 					quoteCount--;
// 					pos++; // Move past the closing quote
// 					break;
// 				}
// 				else if (isFString and text[pos] == '{') {
// 					tokens.append(Token(TokenType::String, stringStart, pos - stringStart, text.mid(stringStart, pos - stringStart)));
// 					pos++; // Move past the '{'
// 					this->tokenize(text);
// 					stringStart = pos;
// 				}
// 				else {
// 					pos++; // Move to the next character
// 				}
// 			}

// 			// Append the remaining string content
// 			if (stringStart < pos) {
// 				tokens.append(Token(TokenType::String, stringStart, pos - stringStart, text.mid(stringStart, pos - stringStart)));
// 			}
// 			if (quoteCount <= 0) {
// 				// أي تم الإنتهاء زالخروج من النص المنسق
// 				isFString = false;
// 			}
// 		}
// 		else if (currentChar == '#') {
// 			int start = pos;
// 			while (pos < text.length() and text[pos] != '\n') {
// 				pos++;
// 			}
// 			tokens.append(Token(TokenType::Comment, start, pos - start, text.mid(start, pos - start)));
// 		}
// 		else if (QString("+-*/\\=<>!&|%^~").contains(currentChar)) {
// 			int start = pos;
// 			QString op;
// 			op += currentChar;
// 			pos++;

// 			if (pos < text.length()) {
// 				QChar nextChar = text[pos];
// 				if ((currentChar == '=' and nextChar == '=') or
// 					(currentChar == '!' and nextChar == '=') or
// 					(currentChar == '<' and nextChar == '=') or
// 					(currentChar == '>' and nextChar == '=')) {
// 					op += nextChar;
// 					pos++;
// 				}
// 			}

// 			tokens.append(Token(TokenType::Operator, start, pos - start, op));
// 		}
// 		else {
// 			pos++;

// 			if (isFString and currentChar == '}') {
// 				// عندما يكون داخل قوس متعرج داخل نص منسق
// 				// يجب التحقق هل إنتهت الحالة
// 				// لكي ينهي عملية البحث
// 				return tokens;
// 			}
// 		}
// 	}
// 	return tokens;
// }

QVector<Token> Lexer::tokenize(const QString& text) {
    // Reset state on each tokenization call
    pos = 0;
    tokens.clear();
    quoteCount = 0;
    isFString = false;

    while (pos < text.length()) {
        QChar currentChar = text[pos];

        if (currentChar.isSpace()) {
            while (pos < text.length() && text[pos].isSpace()) pos++;
        }
        else if (currentChar.isDigit()) {
            int start = pos;
            while (pos < text.length() && (text[pos].isDigit() || text[pos] == '.')) pos++;
            tokens.append(Token(TokenType::Number, start, pos - start, text.mid(start, pos - start)));
        }
        else if (currentChar.isLetter() || currentChar == '_') {
            int start = pos;
            while (pos < text.length() && (text[pos].isLetterOrNumber() || text[pos] == '_')) pos++;

            // Normalize identifier for consistent comparisons (helps مع الهمزات/تشكيل إذا احتجت لاحقًا)
            QString identifier = text.mid(start, pos - start).normalized(QString::NormalizationForm_KC);

            TokenType type = isKeyword(identifier) ? TokenType::Keyword :
                                 isKeyword1(identifier) ? TokenType::Keyword1 :
                                 isKeyword2(identifier) ? TokenType::Keyword2 :
                                 TokenType::Identifier;
            tokens.append(Token(type, start, pos - start, identifier));
        }
        else if (currentChar == '"' || currentChar == '\'') {
            int stringStart = pos;
            quoteCount++;
            // detect f-string marker 'م' before quote (exactly as in JSON delims)
            bool startsWithM = (stringStart > 0 && text[stringStart - 1] == QChar(u'م'));
            if (startsWithM) isFString = true;

            QChar delim = currentChar;
            pos++; // skip opening quote

            while (pos < text.length()) {
                if (text[pos] == '\\') {
                    pos += 2; // skip escape + escaped char (naive but works for highlighter)
                }
                else if (text[pos] == delim) {
                    quoteCount--;
                    pos++; // skip closing quote
                    break;
                }
                else if (isFString && text[pos] == '{') {
                    // Push string part up to the '{'
                    if (stringStart < pos) {
                        tokens.append(Token(TokenType::String, stringStart, pos - stringStart, text.mid(stringStart, pos - stringStart)));
                    }
                    pos++; // skip '{'
                    // Now parse the expression inside braces *without* recursive tokenize()
                    int exprStart = pos;
                    int braceDepth = 1;
                    while (pos < text.length() && braceDepth > 0) {
                        if (text[pos] == '\\') pos += 2;
                        else if (text[pos] == '{') { braceDepth++; pos++; }
                        else if (text[pos] == '}') { braceDepth--; if (braceDepth==0) break; pos++; }
                        else pos++;
                    }
                    // add the interpolation expression as Identifier/other tokens or as String Substitution
                    if (exprStart < pos) {
                        tokens.append(Token(TokenType::Identifier, exprStart, pos - exprStart, text.mid(exprStart, pos - exprStart)));
                    }
                    if (pos < text.length() && text[pos] == '}') pos++; // skip final '}'
                    stringStart = pos; // continue string after interpolation
                }
                else {
                    pos++;
                }
            }

            if (stringStart < pos) {
                tokens.append(Token(TokenType::String, stringStart, pos - stringStart, text.mid(stringStart, pos - stringStart)));
            }
            if (quoteCount <= 0) {
                isFString = false;
                quoteCount = 0;
            }
        }
        else if (currentChar == '#') {
            int start = pos;
            while (pos < text.length() && text[pos] != '\n') pos++;
            tokens.append(Token(TokenType::Comment, start, pos - start, text.mid(start, pos - start)));
        }
        else if (QString("+-*/\\=<>!&|%^~:;(),.[]{}").contains(currentChar)) {
            int start = pos;
            QString op;
            op += currentChar;
            pos++;
            // Check two-char ops
            if (pos < text.length()) {
                QChar nextChar = text[pos];
                if ((currentChar == '=' && nextChar == '=') ||
                    (currentChar == '!' && nextChar == '=') ||
                    (currentChar == '<' && nextChar == '=') ||
                    (currentChar == '>' && nextChar == '=')) {
                    op += nextChar;
                    pos++;
                }
            }
            tokens.append(Token(TokenType::Operator, start, pos - start, op));
        }
        else {
            pos++;
        }
    }

    return tokens;
}


bool Lexer::isKeyword(const QString& word) {
	static const QSet<QString> keywords = {
	"ك", "و", "في", "او", "أو", "من", "مع", "صح", "هل",
	"اذا", "إذا", "ليس", "مرر", "عدم", "ولد", "صنف", "خطا", "خطأ", "عام",
	"احذف", "دالة", "لاجل", "لأجل", "لكل", "والا", "وإلا", "توقف", "نطاق", "ارجع",
	"اواذا", "أوإذا", "بينما", "انتظر", "استمر", "مزامنة", "استورد",
	 "حاول" , "خلل" , "نهاية"
	};
	return keywords.contains(word);
}


bool Lexer::isKeyword1(const QString& word) {
	static const QSet<QString> keywords = {
		"اطبع", "ادخل", "مدى"
	};
	return keywords.contains(word);
}

bool Lexer::isKeyword2(const QString& word) {
	static const QSet<QString> keywords = {
		"_تهيئة_", "هذا", "اصل"
	};
	return keywords.contains(word);
}
