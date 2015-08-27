--
-- Created by IntelliJ IDEA.
-- User: alex
-- Date: 27/08/2015
-- Time: 12:29
-- To change this template use File | Settings | File Templates.
--
local root_dir = getenv("ROOT_DIR")

local handle, msg = loadlib(getenv("LIBRARY_PATH"))
print(handle, msg)

if (not handle or handle == -1) then
    error(msg)
end

callfromlib(handle, 'lua_larchiveopen')