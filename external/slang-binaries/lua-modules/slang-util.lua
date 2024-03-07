-- Define the module in the style for Lua 5.1+

local slangUtil =  {}

function slangUtil.valueToString(o)
    if type(o) == 'table' then
        return slangUtil.tableToString(o)
    else
        return tostring(o)
     end
end
    
function slangUtil.tableToString(o)
    local s = '{ '
    for k,v in pairs(o) do
        if type(k) ~= 'number' then k = '"'..k..'"' end
        s = s .. '['..k..'] = ' .. tostring(v) .. ',\n'
    end
    return s .. '} '
end

function slangUtil.dump(o)
    print(slangUtil.valueToString(o))
end

function slangUtil.trimPrefix(s, p)
    local t = (s:sub(0, #p) == p) and s:sub(#p + 1) or s
    return t
end


-- 
-- Append (assuming 'array' table b onto a)
--
function slangUtil.appendTable(a, b)  
    for _,v in ipairs(b) do 
        table.insert(a, v)
    end
end

function slangUtil.shallowCopyTable(t)
  local u = { }
  for k, v in pairs(t) 
  do 
    u[k] = v 
  end
  return setmetatable(u, getmetatable(t))
end

--
-- Given two (array) tables returns the concatination 
--
function slangUtil.concatTables(a, b)
    a = slangUtil.shallowCopyTable(a)
    slangUtil.appendTable(a, b)
    return a
end

function slangUtil.findLibraries(targetInfo, basePath, inMatchName, matchFunc)
    local matchName = inMatchName
    if targetInfo.isWindows then
        matchName = inMatchName .. ".lib"
    else
        matchName = "lib" .. inMatchName .. ".a"
    end
 
    local matchPath = path.join(basePath, matchName)
 
    local libs = os.matchfiles(matchPath)
       
    local dstLibs = {}   
       
    for k, v in ipairs(libs) do
        -- Strip off path and extension
        local libBaseName = path.getbasename(v)
        local libName = libBaseName
        
        if not targetInfo.isWindows then
            -- If the name starts with "lib" strip it
            libName = slangUtil.trimPrefix(libName, "lib")
        end
    
        if matchFunc == nil or matchFunc(libName) then
            table.insert(dstLibs, libName)
        end
    end
        
    return dstLibs
end

function slangUtil.toBool(v)
    if type(v) == "boolean" then 
        return v
    end
    if  v == "True" or v == "true" then
        return true
    end
    if v == "False" or v == "false" or v == nil then
        return false
    end
    -- Returns nil as an error
    return nil
end

function slangUtil.getBoolOption(name)
    local v = _OPTIONS[name]
    local b = slangUtil.toBool(v)
    if b == nil then
        return error("Option '" .. name .. "' is '" .. v .. "' - not a valid boolean value")
    end
    return b
end

-- Returns the version number of Lua embedded into premake
-- Last time checked it was 5.3

function slangUtil.getLuaVersion()
    return _G._VERSION 
end

function slangUtil.getExecutableSuffix(targetInfo)
    if targetInfo.osTarget == "windows" then
        return ".exe"
    end
    return ""
end

-- 
-- Given an array return as a 'set' (table with entries equating to true)
--
function slangUtil.arrayToSet(arr)
    local t = {}
    for i, value in ipairs(arr) 
    do
        t[value] = true
    end
    return t
end

-- 
-- Split string by the *character* delimiter
--
function slangUtil.splitString(s, delimiter)
    if delimiter == nil then
        delimiter = "%s"
    end
    
    local t = {}
  
    for str in string.gmatch(s, "([^" .. delimiter .. "]+)") 
    do
        table.insert(t, str)
    end
    return t
end

--
-- Given a library name, returns the operating system specific
-- shared library/dll filename
--
function slangUtil.getSharedLibraryFileName(targetInfo, name)
    local osName = targetInfo.os
    if osName == "cygwin" or osName == "windows" or osName == "mingw" then
        return name .. ".dll"
    elseif osName == "macosx" then
        return "lib" .. name .. ".dylib"
    else
        return "lib" .. name .. ".so"
    end    
end

-- 
-- Returns a table with the following values
-- tokenName - The target name with premake tokens 
-- isWindows - True if is windows
-- targetDetail - Target detail as set as the command line parameter (cygwin/mingw)
-- arch - Architecture to be built for (x86_64, x86, arm)
-- name - combination of os and arch
-- os - The name of the os (takes into account targetDetail)
-- osTarget - The target name as returned by os.target 
function slangUtil.getTargetInfo()
    local info = {}

    local targetDetail = _OPTIONS["target-detail"]
    
    -- Os target can be one of 
    -- https://premake.github.io/docs/system/
    -- aix, bsd, haiku, linux, macosx, solaris, wii, windows, xbox360
    
    local osTarget = os.target()

    info.isWindows = (osTarget == "windows") and not (targetDetail == "mingw" or targetDetail == "cygwin")
    info.targetDetail = targetDetail

    local osName = osTarget
    
    local tokenName = "%{cfg.system}-%{cfg.platform:lower()}"
    
    if not (targetDetail == nil) then
        tokenName = targetDetail .. "-%{cfg.platform:lower()}"
        osName = targetDetail
    end

    info.tokenName = tokenName
    
    -- 
    -- We want to have the name such that it can identify a package. 
    -- For that work we need to know the arch that is being built. 
    -- Here we rely on that being passed in as --arch option
    --
    local arch = _OPTIONS["arch"]
    if arch == nil then
        arch = "x86_64"
    else      
        local valueMap = 
        {
            -- We allow Win32 as thats a convenient way to specify x86 on windows
            win32 = "x86",
            -- We allow x64 to mean x86_64 
            x64 = "x86_64",
        }
        
        local value = valueMap[arch:lower()]
        
        if value ~= nil then
            arch = value
        end
    end
    
    info.arch = arch
    info.name = osName .. "-" .. arch
    info.os = osName
    info.osTarget = osTarget
    
    return info
end

function slangUtil.getPremakeArchName(arch)
    -- Returns an arch name as used by premake project
    local tab = {
        x86_64 = "x64"
    }
    
    local v = tab[arch]
    if v == nil then 
        return arch
    else
        return v
    end 
end

--
-- Get the name visual studio uses for 'platform' from the specified arch
--

function slangUtil.getVisualStudioPlatformName(arch)
    local valueMap = 
    {
        x86 = "win32",
        x86_64 = "x64",
    }
    
    local value = valueMap[arch:lower()]
    if value == nil then
        return arch
    else
        return value
    end
end

-- A function to return a name to place project files under 
-- in build directory
--
-- This is complicated in so far as when this is used (with location for example)
-- we can't use Tokens 
-- https://github.com/premake/premake-core/wiki/Tokens

function slangUtil.getBuildLocationName(targetInfo)
    -- This might need to be updated 
    if targetInfo.isWindows then
        return "visual-studio"
    else
        -- Using 'name' - but if arch is not set will be confusing.
        return targetInfo.os
    end 
end

-- Importing the module should make options automatically available

newoption {
   trigger     = "target-detail",
   description = "(Optional) More specific target information",
   value       = "string",
   allowed     = { {"cygwin"}, {"mingw"} }
}

newoption {
   trigger     = "arch",
   description = "(Optional) The arch that is going to be built",
   value       = "string",
   allowed     = { {"x86_64"}, {"x86"}, {"arm"}, {"aarch64"}, {"Win32", "Same as x86"}, {"win32", "Same as x86"}, {"x64", "Same as x86_64" } }
}

return slangUtil