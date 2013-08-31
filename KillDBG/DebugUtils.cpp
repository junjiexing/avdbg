#include "stdafx.h"
#include "DebugUtils.h"

bool debug_utils::get_error_msg( DWORD error_code,std::string& message )
{
	HLOCAL hlocal = NULL;   // Buffer that gets the error message string

	// Use the default system locale since we look for Windows messages.
	// Note: this MAKELANGID combination has 0 as value
	DWORD system_locale = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

	// Get the error code's textual description
	BOOL result = FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |FORMAT_MESSAGE_ALLOCATE_BUFFER, 
		NULL, error_code, system_locale, (PTSTR) &hlocal, 0, NULL);

	if (!result)
	{
		// Is it a network-related error?
		HMODULE hDll = LoadLibraryEx(TEXT("netmsg.dll"), NULL, DONT_RESOLVE_DLL_REFERENCES);

		if (hDll != NULL)
		{
			result = FormatMessage(
				FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS |FORMAT_MESSAGE_ALLOCATE_BUFFER,
				hDll, error_code, system_locale, (PTSTR) &hlocal, 0, NULL);
			FreeLibrary(hDll);
		}
	}

	if (result && (hlocal != NULL))
	{
		message = (PCTSTR) LocalLock(hlocal);
		LocalFree(hlocal);
		return true;
	}

	return false;
}

bool debug_utils::get_file_name_frome_handle( HANDLE hfile, std::string& file_name )
{
	bool ret = false;
	char buffer[MAX_PATH];

	// Get the file size.
	DWORD file_size_high = 0;
	DWORD file_size_low = GetFileSize(hfile,&file_size_high); 

	// Create a file mapping object.
	HANDLE file_map_handle = CreateFileMapping(hfile,NULL,PAGE_READONLY,0,file_size_low,NULL);

	if (file_map_handle) 
	{
		// Create a file mapping to get the file name.
		void* mem_ptr = MapViewOfFile(file_map_handle, FILE_MAP_READ, 0, 0, 1);

		if (mem_ptr) 
		{
			if (GetMappedFileName(GetCurrentProcess(), mem_ptr, buffer, MAX_PATH)) 
			{
				// Translate path with device name to drive letters.
				char driver_string[512];
				driver_string[0] = '\0';

				if (GetLogicalDriveStrings(512-1, driver_string)) 
				{
					char target_path[MAX_PATH];
					char device_name[3] = " :";
					bool found = false;
					char* p = driver_string;

					do 
					{
						// Copy the drive letter to the template string
						*device_name = *p;

						// Look up each device name
						if (QueryDosDevice(device_name, target_path, 512))
						{
							UINT name_len = strlen(target_path);

							if (name_len < MAX_PATH) 
							{
								found = _strnicmp(buffer, target_path, name_len) == 0;

								if (found) 
								{
									// Reconstruct pszFilename using szTemp
									// Replace device path with DOS path
									char temp_file_name[MAX_PATH];
									sprintf(temp_file_name,"%s%s",device_name,buffer+name_len);
									strncpy(buffer, temp_file_name, MAX_PATH);
								}
							}
						}

						// Go to the next NULL character.
						while (*p++);
					} while (!found && *p); // end of string
				}
			}
			ret = true;
			UnmapViewOfFile(mem_ptr);
		} 

		CloseHandle(file_map_handle);
	}

	file_name = buffer;

	return ret;
}

bool debug_utils::get_word_from_pos( const std::string& src,int pos,int& start,int& end )
{
	if (pos>src.size() || pos<0)
	{
		return false;
	}

	bool found_start = false;
	bool found_end = false;
	for (int i=0;i<src.size();++i)
	{
		if (is_bound(src[i]))
		{
			if (i<pos)
			{
				start = i+1;
				found_start = true;
			}
			else if (i>=pos)
			{
				end = i;
				found_end = true;
				break;
			}
		}
	}

	if (!found_start)
	{
		start = 0;
	}

	if (!found_end)
	{
		end = src.size();
	}

	return true;
}

bool debug_utils::is_bound( char ch )
{
	return ch == '%' || ch == '^' || ch == '&' || ch == '*' ||
		ch == '(' || ch == ')' || ch == '-' || ch == '+' ||
		ch == '=' || ch == '|' || ch == '{' || ch == '}' ||
		ch == '[' || ch == ']' || ch == ':' || ch == ';' ||
		ch == '<' || ch == '>' || ch == ',' || ch == '/' ||
		ch == '?' || ch == '!' || ch == '.' || ch == '~' || ch == ' ';
}
