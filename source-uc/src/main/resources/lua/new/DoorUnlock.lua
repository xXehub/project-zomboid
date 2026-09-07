DoorUnlock = {}

DoorUnlock.toggle = function(worldobjects, door)
    ToggleDoor(door)
end

DoorUnlock.destroy = function(worldobjects, door)
    Destroy(door)
end

DoorUnlock.destroyObj = function(worldobjects, obj)
    DestroyObject(obj)
end

DoorUnlock.moveObj = function(worldobjects, obj)
    MoveObject(obj)
end

DoorUnlock.onDoorLock = function(worldobjects, door)    
    door:setIsLocked(not door:isLocked())    
    if instanceof(door, "IsoDoor") and door:checkKeyId() ~= -1 then
        door:setLockedByKey(door:isLocked())
    end
    if instanceof(door, "IsoThumpable") and door:getKeyId() ~= -1 then
        door:setLockedByKey(door:isLocked())
    end
    getPlayer():getMapKnowledge():setKnownBlockedDoor(door, door:isLocked())

    local doubleDoorObjects = buildUtil.getDoubleDoorObjects(door)
    for i=1,#doubleDoorObjects do
        local object = doubleDoorObjects[i]
        object:setLockedByKey(door:isLocked())
    end

    local garageDoorObjects = buildUtil.getGarageDoorObjects(door)
    for i=1,#garageDoorObjects do
        local object = garageDoorObjects[i]
        object:setLockedByKey(door:isLocked())
    end
end

local function removeDuplicates(list)
    local result = {}
    local seen = {}
    for _,item in ipairs(list) do
        if not seen[item] then
            seen[item] = true
            table.insert(result, item)
        end
    end
    return result
end

DoorUnlock.doMenu = function(player, context, worldobjects, test)    
    local square = nil;
    for i,v in ipairs(worldobjects) do
        square = v:getSquare();
        break;
    end

    for i=1,square:getObjects():size() do
        table.insert(worldobjects, square:getObjects():get(i-1))
    end
    worldobjects = removeDuplicates(worldobjects)

    local playerObj = getSpecificPlayer(player)

    local subMenu = ISContextMenu:getNew(context);

    for _,obj in ipairs(worldobjects) do        
        if instanceof(obj, "IsoDoor") or (instanceof(obj, "IsoThumpable") and obj:isDoor()) then            
            context:addOption(obj:isLocked() and "Door Unlock" or "Door Lock", worldobjects, DoorUnlock.onDoorLock, obj)
            if instanceof(obj, "IsoDoor") then         
                context:addOption(obj:isOpen() and "XXXClose" or "XXXOpen", worldobjects, DoorUnlock.toggle, obj)
                context:addOption("Destroy", worldobjects, DoorUnlock.destroy, obj)
            else
                context:addOption("Destroy Puerta", worldobjects, DoorUnlock.destroyObj, obj)
            end
        elseif instanceof(obj, "IsoObject") then
            context:addOption("Destroy " .. obj:getSpriteName(), worldobjects, DoorUnlock.destroyObj, obj)
            context:addOption("MoveObject " .. obj:getSpriteName(), worldobjects, DoorUnlock.moveObj, obj)            
        end
    end
end



ISWorldObjectContextMenu.createMenu = function(player, worldobjects, x, y, test)
	local timeStamp = getTimestampMs();

	if ISWorldObjectContextMenu.disableWorldMenu then
		return;
	end
	if getCore():getGameMode() == "Tutorial" then
		local context = Tutorial1.createWorldContextMenu(player, worldobjects, x ,y);
		return context;
	end	
	if UIManager.getSpeedControls():getCurrentGameSpeed() == 0 then
		return;
	end

	local playerObj = getSpecificPlayer(player)
	if playerObj:isAsleep() then return end

    local context = ISContextMenu.get(player, x, y);
	context.troughSubmenu = nil;
	context.dontShowLiquidOption = false;
    
    if ISTradingUI.instance and ISTradingUI.instance:isVisible() then
        context:addOption(getText("IGUI_TradingUI_CantRightClick"), nil, nil);
        return;
	end

    context.blinkOption = ISWorldObjectContextMenu.blinkOption;

    if test then context:setVisible(false) end
    ISWorldObjectContextMenu.Test = false

	getCell():setDrag(nil, player);

	ISWorldObjectContextMenu.clearFetch()
	local fetch = ISWorldObjectContextMenu.fetchVars

	local fetchStartTime = getTimestampMs();
    for i,v in ipairs(worldobjects) do
		ISWorldObjectContextMenuLogic.fetch(fetch, v, player, true);
	end

	triggerEvent("OnPreFillWorldObjectContextMenu", player, context, worldobjects, test);

    if fetch.c == 0 then
        return;
    end

    for _,tooltip in ipairs(ISWorldObjectContextMenu.tooltipsUsed) do
        table.insert(ISWorldObjectContextMenu.tooltipPool, tooltip);
    end
    table.wipe(ISWorldObjectContextMenu.tooltipsUsed);

    for _,tooltip in ipairs(ISWorldObjectContextMenu.tooltipInvUsed) do
        table.insert(ISWorldObjectContextMenu.tooltipInvPool, tooltip);
    end
    table.wipe(ISWorldObjectContextMenu.tooltipInvUsed);

    local pickedCorpse = IsoObjectPicker.Instance:PickCorpse(x, y)
    fetch.body = pickedCorpse or fetch.body

	if ISWorldObjectContextMenuLogic.createMenuEntries(fetch, context, player, worldobjects, x, y, test or false) then return true end

    if fetch.safehouseAllowInteract then        
        triggerEvent("OnFillWorldObjectContextMenu", player, context, worldobjects, test);
    end

    DoorUnlock.doMenu(player, context, worldobjects, test)

    if test then return ISWorldObjectContextMenu.Test end

    if context.numOptions == 1 then
        context:setVisible(false);
    end

    return context;
end

