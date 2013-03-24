/**\file
 * Archive wrapper
 */
#ifndef __EXTRACT_HPP__
#define __EXTRACT_HPP__

#include <vector>

#include <Windows.h>

extern const char extractCopyright[];

/// Helper to extract 7-zip archives
void Extract (const std::vector<const wchar_t*>& archives, const wchar_t* targetDir, std::vector<std::wstring>& extractedFiles);

#endif // __EXTRACT_HPP__
