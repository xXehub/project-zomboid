function ISVehicleMenu.createKey(playerObj, vehicle)
	CreateKeyVehicle(vehicle)
end

function ISVehicleMenu.destroyVehicle(playerObj, vehicle)
	DestroyVehicle(vehicle)
end


local ogFillPartMenu = ISVehicleMenu.FillPartMenu

function ISVehicleMenu.FillPartMenu(playerIndex, context, slice, vehicle)
	ogFillPartMenu(playerIndex, context, slice, vehicle)
	local playerObj = getSpecificPlayer(playerIndex);
	if playerObj:DistToProper(vehicle) >= 4 then
		return
	end
	local typeToItem = VehicleUtils.getItems(playerIndex)
	for i=1,vehicle:getPartCount() do
		local part = vehicle:getPartByIndex(i-1)
		if not vehicle:isEngineStarted() and part:isContainer() and part:getContainerContentType() == "Gasoline" then			
			if slice then
				slice:addSlice(getText("ContextMenu_VehicleRefuelFromPump"), getTexture("media/ui/vehicles/vehicle_refuel_from_pump.png"), ISVehiclePartMenu.onPumpGasoline, playerObj, part)
			else				
				context:addOption("Create Key", playerObj, ISVehicleMenu.createKey, vehicle)				
				local gas = ISVehiclePartMenu.doSiphonFuelMenu(playerObj, part, context)
			end
		end
	end
end

function ISVehiclePartMenu.onTakeGasoline(playerObj, part)
	if playerObj:getVehicle() then
		ISVehicleMenu.onExit(playerObj)
	end
	local typeToItem,tagToItem = VehicleUtils.getItems(playerObj:getPlayerNum())
	local item = ISVehiclePartMenu.getGasCanNotFull(playerObj, typeToItem)
	local hose = tagToItem[ItemTag.SIPHON_GAS] and tagToItem[ItemTag.SIPHON_GAS][1]
	if item then
		ISVehiclePartMenu.toPlayerInventory(playerObj, item)
		ISTimedActionQueue.add(ISPathFindAction:pathToVehicleArea(playerObj, part:getVehicle(), part:getArea()))
		ISInventoryPaneContextMenu.equipWeapon(item, false, false, playerObj:getPlayerNum())
		ISTimedActionQueue.add(ISTakeGasolineFromVehicle:new(playerObj, part, item))
	end
end

ISVehiclePartMenu.doSiphonFuelMenu = function(playerObj, part, context)
	local source = part:getVehicle()
	local playerNum = playerObj:getPlayerNum()
	local playerInv = playerObj:getInventory()
	local hose = playerObj:getInventory():getFirstTagRecurse(ItemTag.SIPHON_GAS)
	local allContainers = {}
	local allContainerTypes = {}
	local allContainersOfType = {}
	local pourInto = playerInv:getAllEvalRecurse(function(item)		
		if item:getFluidContainer() and item:getFluidContainer():isEmpty() then
			return true
		end		
		if item:getFluidContainer() and item:getFluidContainer():contains(Fluid.Petrol) and item:getFluidContainer():getFreeCapacity() > 0 then
			return true
		end
		return false
	end)
	if pourInto:isEmpty() then
		return
	end
	local fillOption = context:addOption(getText("ContextMenu_VehicleSiphonGas"), worldobjects, nil);
	if playerObj:hasFullInventory() then
		fillOption.notAvailable = true;
		local tooltip = ISInventoryPaneContextMenu.addToolTip();
		tooltip.description = getText("ContextMenu_FullInventory");
		fillOption.toolTip = tooltip;
		return false
	end							
	if not source:getSquare() or not AdjacentFreeTileFinder.Find(source:getSquare(), playerObj) then
		fillOption.notAvailable = true;		
		return;
	end	
	for i=0, pourInto:size() - 1 do
		local container = pourInto:get(i)
		table.insert(allContainers, container)
	end		
	table.sort(allContainers, function(a,b) return not string.sort(a:getName(), b:getName()) end)	
	local previousContainer = nil;
	for _,container in pairs(allContainers) do
		if previousContainer ~= nil and container:getName() ~= previousContainer:getName() then
			table.insert(allContainerTypes, allContainersOfType)
			allContainersOfType = {}
		end
		table.insert(allContainersOfType, container)
		previousContainer = container
	end
	table.insert(allContainerTypes, allContainersOfType)	
	local containerMenu = ISContextMenu:getNew(context)
	local containerOption
	context:addSubMenu(fillOption, containerMenu)
	if pourInto:size() > 1 then
		containerOption = containerMenu:addOption(getText("ContextMenu_FillAll"), worldobjects, ISVehiclePartMenu.onTakeFuelNew, part, allContainers, nil, playerNum);
	end	
	for _,containerType in pairs(allContainerTypes) do
		local destItem = containerType[1]
	if #containerType > 1 then 
			containerOption = containerMenu:addOption(destItem:getName() .. " (" .. #containerType ..")", worldobjects, nil);
			local containerTypeMenu = ISContextMenu:getNew(containerMenu)
			containerMenu:addSubMenu(containerOption, containerTypeMenu)
			local containerTypeOption
			containerTypeOption = containerTypeMenu:addOption(getText("ContextMenu_FillOne"), worldobjects, ISVehiclePartMenu.onTakeFuelNew, part, nil, destItem, playerNum);
			if containerType[2] ~= nil then
				containerTypeOption = containerTypeMenu:addOption(getText("ContextMenu_FillAll"), worldobjects, ISVehiclePartMenu.onTakeFuelNew, part, containerType, nil, playerNum);
			end
		else
			containerOption = containerMenu:addOption(destItem:getName(), worldobjects, ISVehiclePartMenu.onTakeFuelNew, part, nil, destItem, playerNum);
			if destItem:getFluidContainer() then
				local t = ISWorldObjectContextMenu.addToolTip()
				t.maxLineWidth = 512
				t.description = getText("ContextMenu_FuelCapacity") .. string.format("%s / %s", (math.floor(destItem:getFluidContainer():getFreeCapacity() * 1000) / 1000), (math.floor(destItem:getFluidContainer():getCapacity() * 1000) / 1000))
				containerOption.toolTip = t
			end
		end
	end
end


function ISVehicleMenu.showRadialMenu(playerObj)
	local isPaused = UIManager.getSpeedControls() and UIManager.getSpeedControls():getCurrentGameSpeed() == 0
	if isPaused then return end

	local vehicle = playerObj:getVehicle()
	if not vehicle then
		ISVehicleMenu.showRadialMenuOutside(playerObj)
		return
	end

	local menu = getPlayerRadialMenu(playerObj:getPlayerNum())
	menu:clear()

	if menu:isReallyVisible() then
		if menu.joyfocus then
			setJoypadFocus(playerObj:getPlayerNum(), nil)
		end
		menu:undisplay()
		return
	end

	menu:setX(getPlayerScreenLeft(playerObj:getPlayerNum()) + getPlayerScreenWidth(playerObj:getPlayerNum()) / 2 - menu:getWidth() / 2)
	menu:setY(getPlayerScreenTop(playerObj:getPlayerNum()) + getPlayerScreenHeight(playerObj:getPlayerNum()) / 2 - menu:getHeight() / 2)

	local texture = Joypad.Texture.AButton
	
	local seat = vehicle:getSeat(playerObj)

	menu:addSlice(getText("IGUI_SwitchSeat"), getTexture("media/ui/vehicles/vehicle_changeseats.png"), ISVehicleMenu.onShowSeatUI, playerObj, vehicle )

	if vehicle:isDriver(playerObj) and vehicle:isEngineWorking() then
		if vehicle:isEngineRunning() then
			menu:addSlice(getText("ContextMenu_VehicleShutOff"), getTexture("media/ui/vehicles/vehicle_ignitionOFF.png"), ISVehicleMenu.onShutOff, playerObj)
		else
			if vehicle:isEngineStarted() then
			else
				if (SandboxVars.VehicleEasyUse) then
					menu:addSlice(getText("ContextMenu_VehicleStartEngine"), getTexture("media/ui/vehicles/vehicle_ignitionON.png"), ISVehicleMenu.onStartEngine, playerObj)
				elseif not vehicle:isHotwired() and (playerObj:getInventory():haveThisKeyId(vehicle:getKeyId()) or vehicle:isKeysInIgnition()) then
					menu:addSlice(getText("ContextMenu_VehicleStartEngine"), getTexture("media/ui/vehicles/vehicle_ignitionON.png"), ISVehicleMenu.onStartEngine, playerObj)
				elseif not vehicle:isHotwired() and ((playerObj:getPerkLevel(Perks.Electricity) >= 2 and playerObj:getPerkLevel(Perks.Mechanics) >= 3) or playerObj:hasTrait(CharacterTrait.BURGLAR))then
				elseif vehicle:isHotwired() then
					menu:addSlice(getText("ContextMenu_VehicleStartEngine"), getTexture("media/ui/vehicles/vehicle_ignitionON.png"), ISVehicleMenu.onStartEngine, playerObj)
				else
				end
			end
		end
	end

	if vehicle:isDriver(playerObj) and
			not vehicle:isHotwired() and
			not vehicle:isEngineStarted() and
			not vehicle:isEngineRunning() and
			not SandboxVars.VehicleEasyUse and
			not vehicle:isKeysInIgnition() and
			not playerObj:getInventory():haveThisKeyId(vehicle:getKeyId()) then
		if true then
			menu:addSlice(getText("ContextMenu_VehicleHotwire"), getTexture("media/ui/vehicles/vehicle_ignitionON.png"), ISVehicleMenu.onHotwire, playerObj)
		else
			menu:addSlice(getText("ContextMenu_VehicleHotwireSkill"), getTexture("media/ui/vehicles/vehicle_ignitionOFF.png"), nil, playerObj)
		end
	end

	if vehicle:isDriver(playerObj) and vehicle:hasHeadlights() then
		if vehicle:getHeadlightsOn() then
			menu:addSlice(getText("ContextMenu_VehicleHeadlightsOff"), getTexture("media/ui/vehicles/vehicle_lightsOFF.png"), ISVehicleMenu.onToggleHeadlights, playerObj)
		else
			menu:addSlice(getText("ContextMenu_VehicleHeadlightsOn"), getTexture("media/ui/vehicles/vehicle_lightsON.png"), ISVehicleMenu.onToggleHeadlights, playerObj)
		end
	end

	if vehicle:getPartById("Heater") then
		local tex = getTexture("media/ui/vehicles/vehicle_temperatureHOT.png")
		if (vehicle:getPartById("Heater"):getModData().temperature or 0) < 0 then
			tex = getTexture("media/ui/vehicles/vehicle_temperatureCOLD.png")
		end
		if vehicle:getPartById("Heater"):getModData().active then
			menu:addSlice(getText("ContextMenu_VehicleHeaterOff"), tex, ISVehicleMenu.onToggleHeater, playerObj )
		else
			menu:addSlice(getText("ContextMenu_VehicleHeaterOn"), tex, ISVehicleMenu.onToggleHeater, playerObj )
		end
	end
	
	if vehicle:isDriver(playerObj) and vehicle:hasHorn() then
		menu:addSlice(getText("ContextMenu_VehicleHorn"), getTexture("media/ui/vehicles/vehicle_horn.png"), ISVehicleMenu.onHorn, playerObj)
	end
	
	if (vehicle:hasLightbar()) then
		menu:addSlice(getText("ContextMenu_VehicleLightbar"), getTexture("media/ui/vehicles/vehicle_lightbar.png"), ISVehicleMenu.onLightbar, playerObj)
	end

if seat <= 1 then 
		for partIndex=1,vehicle:getPartCount() do
			local part = vehicle:getPartByIndex(partIndex-1)
			if part:getDeviceData() and part:getInventoryItem() then
				menu:addSlice(getText("IGUI_DeviceOptions"), getTexture("media/ui/vehicles/vehicle_speakersON.png"), ISVehicleMenu.onSignalDevice, playerObj, part)
			end
		end
	end

	local door = vehicle:getPassengerDoor(seat)
	local windowPart = VehicleUtils.getChildWindow(door)
	if windowPart and (not windowPart:getItemType() or windowPart:getInventoryItem()) then
		local window = windowPart:getWindow()
		if window:isOpenable() and not window:isDestroyed() then
			if window:isOpen() then
				local option = menu:addSlice(getText("ContextMenu_Close_window"), getTexture("media/ui/vehicles/vehicle_windowCLOSED.png"), ISVehiclePartMenu.onOpenCloseWindow, playerObj, windowPart, false)
			else
				local option = menu:addSlice(getText("ContextMenu_Open_window"), getTexture("media/ui/vehicles/vehicle_windowOPEN.png"), ISVehiclePartMenu.onOpenCloseWindow, playerObj, windowPart, true)
			end
		end
	end

	local locked = vehicle:isAnyDoorLocked()
	if JoypadState.players[playerObj:getPlayerNum()+1] then		
		locked = locked or vehicle:isTrunkLocked()
	end
	if locked then
		menu:addSlice(getText("ContextMenu_Unlock_Doors"), getTexture("media/ui/vehicles/vehicle_lockdoors.png"), ISVehiclePartMenu.onLockDoors, playerObj, vehicle, false)
	else
		menu:addSlice(getText("ContextMenu_Lock_Doors"), getTexture("media/ui/vehicles/vehicle_lockdoors.png"), ISVehiclePartMenu.onLockDoors, playerObj, vehicle, true)
	end
	
	if not vehicle:isStopped() then
		menu:addSlice(getText("ContextMenu_VehicleMechanicsStopCar"), getTexture("media/ui/vehicles/vehicle_repair.png"), nil, playerObj, vehicle )
	else
		menu:addSlice(getText("ContextMenu_VehicleMechanics"), getTexture("media/ui/vehicles/vehicle_repair.png"), ISVehicleMenu.onMechanic, playerObj, vehicle )
	end
	if (not isClient() or getServerOptions():getBoolean("SleepAllowed")) then
		local doSleep = true;
		local sleepNeeded = not isClient() or getServerOptions():getBoolean("SleepNeeded")
		
	
		local isZombies = playerObj:getStats():getNumVisibleZombies() > 0 or playerObj:getStats():getNumChasingZombies() > 0 or playerObj:getStats():getNumVeryCloseZombies() > 0
		if sleepNeeded and (playerObj:getStats():get(CharacterStat.FATIGUE) <= 0.3) then
			menu:addSlice(getText("IGUI_Sleep_NotTiredEnough"), getTexture("media/ui/vehicles/vehicle_sleep.png"), nil, playerObj, vehicle)
			doSleep = false;
		elseif not vehicle:isStopped() then
			menu:addSlice(getText("IGUI_PlayerText_CanNotSleepInMovingCar"), getTexture("media/ui/vehicles/vehicle_sleep.png"), nil, playerObj, vehicle)
			doSleep = false;
		elseif sleepNeeded and isZombies then
			menu:addSlice(getText("IGUI_Sleep_NotSafe"), getTexture("media/ui/vehicles/vehicle_sleep.png"), nil, playerObj, vehicle)
			doSleep = false;		
		else
			if sleepNeeded and ((playerObj:getHoursSurvived() - playerObj:getLastHourSleeped()) <= 1) then				
				menu:addSlice(getText("ContextMenu_NoSleepTooEarly"), getTexture("media/ui/vehicles/vehicle_sleep.png"), nil, playerObj, vehicle)
				doSleep = false;			
			elseif playerObj:getSleepingTabletEffect() < 2000 then				
				if playerObj:getMoodles():getMoodleLevel(MoodleType.PAIN) >= 2 and playerObj:getStats():get(CharacterStat.FATIGUE) <= 0.85 then
					menu:addSlice(getText("ContextMenu_PainNoSleep"), getTexture("media/ui/vehicles/vehicle_sleep.png"), nil, playerObj, vehicle)
					doSleep = false;					
				elseif playerObj:getMoodles():getMoodleLevel(MoodleType.PANIC) >= 1 then
					menu:addSlice(getText("ContextMenu_PanicNoSleep"), getTexture("media/ui/vehicles/vehicle_sleep.png"), nil, playerObj, vehicle)
					doSleep = false;					
				end
			end
		end
		if doSleep then
			menu:addSlice(getText("ContextMenu_Sleep"), getTexture("media/ui/vehicles/vehicle_sleep.png"), ISVehicleMenu.onSleep, playerObj, vehicle);
		end
	end
	menu:addSlice(getText("IGUI_ExitVehicle"), getTexture("media/ui/vehicles/vehicle_exit.png"), ISVehicleMenu.onExit, playerObj)

	menu:addToUIManager()

	getSoundManager():playUISound("UIVehicleMenuOpen")
menu.sounds.undisplay = "UIVehicleMenuClose" 

	if JoypadState.players[playerObj:getPlayerNum()+1] then
		menu:setHideWhenButtonReleased(Joypad.DPadUp)
		setJoypadFocus(playerObj:getPlayerNum(), menu)
		playerObj:setJoypadIgnoreAimUntilCentered(true)
	end
end