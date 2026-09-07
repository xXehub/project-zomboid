function ISBuildingObject:haveMaterial(square)

    local dragItem = self
    local modData = dragItem.modData;

	local groundItems = buildUtil.getMaterialOnGround(square);
	local groundItemCounts = buildUtil.getMaterialOnGroundCounts(groundItems)
	local groundItemUses = buildUtil.getMaterialOnGroundUses(groundItems)
    
	
	local playerObj = self.character;
    if playerObj == nil and dragItem ~= nil then
        playerObj = getSpecificPlayer(dragItem.player)
    end
	local playerInv = playerObj:getInventory()

    local cheat = playerObj:isBuildCheat();
    if ISBuildMenu and ISBuildMenu.cheat then
        cheat = true;
    end
    if cheat then
        return true;
    end

	if modData ~= nil then
		for index, value in pairs(modData) do
			if luautils.stringStarts(index, "need:") then
				local itemFullType = luautils.split(index, ":")[2];                
                
				local nbOfItem = playerInv:getCountTypeEvalRecurse(itemFullType, buildUtil.predicateMaterial)				
				if itemFullType == "Base.Nails" then
					nbOfItem = nbOfItem + playerInv:getCountTypeEvalRecurse("Base.NailsBox", buildUtil.predicateMaterial)*100;
					if groundItemCounts["Base.NailsBox"] then
						nbOfItem = nbOfItem + groundItemCounts["Base.NailsBox"]*100;
					end
				end
				if groundItemCounts[itemFullType] then
					nbOfItem = nbOfItem + groundItemCounts[itemFullType];
				end
				if nbOfItem < tonumber(value) then
					return false;
				end
			end
			if luautils.stringStarts(index, "use:") then
				local itemFullType = luautils.split(index, ":")[2];
				local nbOfUse = playerInv:getUsesTypeRecurse(itemFullType)
				if groundItemUses[itemFullType] then
					nbOfUse = nbOfUse + groundItemUses[itemFullType];
				end
				if nbOfUse < tonumber(value) then
					return false;
				end
			end
		end
	end
	
	if not self.noNeedHammer and not ISBuildMenu.cheat then
		local hammer = playerInv:getFirstTagEvalRecurse(ItemTag.HAMMER, predicateNotBroken);
		if not hammer then
			return false;
		end
	end
	return true;
end

function ISBuildIsoEntity:isValid(square)
	if self.blockBuild then
		return false;
	end

	if not square then
		return false;
	end
	
	if not self:haveMaterial(square) then return false end
				

	local x,y,z = square:getX(), square:getY(), square:getZ();

	local face = self:getFace();
	if not face then
		return false;
	end
	for zz=0,face:getzLayers()-1 do
		for xx=0,face:getWidth()-1 do
			for yy=0,face:getHeight()-1 do
				local tileInfo = face:getTileInfo(xx,yy,zz);
				local sq = getCell():getGridSquare(x+xx, y+yy, z+zz);
				if tileInfo then
					if not sq then
						return false;
					else
						if not self:isValidPerSquare(sq, tileInfo, zz==0, yy>0, xx>0) then
							return false;
						end
					end
				end
			end
		end
	end
	return true;
end

function ISWidgetBuildControl:prerender()
	if self.background then
		self:drawRectStatic(0, 0, self.width, self.boxHeight, self.backgroundColor.a, self.backgroundColor.r, self.backgroundColor.g, self.backgroundColor.b);
		self:drawRectBorderStatic(0, 0, self.width, self.boxHeight, self.borderColor.a, self.borderColor.r, self.borderColor.g, self.borderColor.b);
	end
    if self.buttonCraft then
        local cheat = self.player:isBuildCheat();
        local canBuild = true;

        if self.logic:isCraftActionInProgress() then
            canBuild = false;
        end
        self.buttonCraft.enable = canBuild;
        self.buttonCraft:setVisible(true);
    end
end

local finish = true

function ISBuildPanel:createBuildIsoEntity(dontSetDrag)    
    local _player = self.player;
    local _info = self.logic:getSelectedBuildObject();
    local _recipe = self.logic:getRecipe();
    local _wallCoveringParams = self.logic:getWallCoveringParams();

    if _info ~= nil and _recipe ~= nil then

        if self.buildEntity == nil or self.buildEntity.objectInfo ~= _info then
            local containers = ISInventoryPaneContextMenu.getContainers(self.player)
            self.buildEntity = ISBuildIsoEntity:new(_player, _info, 1, containers, self.logic);
            self.buildEntity.dragNilAfterPlace = false;
            self.buildEntity.blockAfterPlace = true;

            local inventory = _player:getInventory();
            self.buildEntity.equipBothHandItem = getTool(_recipe:getToolBoth(), inventory);
            self.buildEntity.firstItem = getTool(_recipe:getToolRight(), inventory);
            self.buildEntity.secondItem = getTool(_recipe:getToolLeft(), inventory);
        end

    	local cheat = self.player:isBuildCheat();
        local canBuild = self.logic:canPerformCurrentRecipe() or cheat;

        if self.logic:isCraftActionInProgress() then
            canBuild = false;
        end

        self.buildEntity.blockBuild = not canBuild;

        if not dontSetDrag then
            getCell():setDrag(self.buildEntity, _player:getPlayerNum());
        end
    elseif _wallCoveringParams ~= nil and _recipe ~= nil then
        if _wallCoveringParams.actionType == WallCoveringType.WALLPAPER then            
            self.buildEntity = ISPaperCursor:new(_player, _wallCoveringParams.wallpaperType, WallPaper["wall"][_wallCoveringParams.wallpaperType]);
            getCell():setDrag(self.buildEntity, _player:getPlayerNum());
        else            
            self.buildEntity = ISPaintCursor:new(_player, _wallCoveringParams.actionType:toString(), _wallCoveringParams);
            getCell():setDrag(self.buildEntity, _player:getPlayerNum());
        end
    else        
        self.buildEntity = nil;
        getCell():setDrag(nil, _player:getPlayerNum());
    end
end

function buildUtil.consumeMaterial(ISItem)    
	if not ISItem or not ISItem.player then return {}; end
	if not isServer() and ISBuildMenu.cheat then
		return {};
	end
	local playerObj = ISItem.player
	if not isServer() then
	    playerObj = getSpecificPlayer(ISItem.player)
	end
	local playerInv = playerObj:getInventory()
	local modData = ISItem.modData;
	local removedFromGround = false
	local consumedItems = {}
	for index, value in pairs(modData) do
		if luautils.stringStarts(index, "need:") then
			local itemFullType = luautils.split(index, ":")[2];
			local itemType = luautils.split(itemFullType, "%.")[2]
			local itemCount = tonumber(value);
			local items = playerInv:getSomeTypeEvalRecurse(itemFullType, buildUtil.predicateMaterial, itemCount)
			for i=1,items:size() do
				local item = items:get(i-1)
				playerObj:removeFromHands(item)
				if item:getContainer() then
					item:getContainer():Remove(item);
					sendRemoveItemFromContainer(item:getContainer(), item);
				else
					playerInv:Remove(item)
					sendRemoveItemFromContainer(playerInv, item);
				end
				itemCount = itemCount - 1
				table.insert(consumedItems, item)
			end						
			if itemCount > 0 then				
				local groundItems = buildUtil.getMaterialOnGround(ISItem.sq)
				local items = groundItems[itemFullType]
				if items then
					local count = math.min(itemCount, #items)
					for i=1,count do
						local item = items[i]
						local worldObj = item:getWorldItem()
						table.insert(consumedItems, item)
						worldObj:getSquare():transmitRemoveItemFromSquare(worldObj)
					end
					itemCount = itemCount - count
					removedFromGround = true
				end
			end
			if itemCount > 0 and itemFullType == "Base.Nails" then
				buildUtil.openNailsBox(ISItem)
				items = playerInv:getSomeTypeEvalRecurse(itemFullType, buildUtil.predicateMaterial, itemCount)
				for i=1,items:size() do
					local item = items:get(i-1)
					playerObj:removeFromHands(item)
					if item:getContainer() then
						item:getContainer():Remove(item);
						sendRemoveItemFromContainer(item:getContainer(), item);
					else
						playerInv:Remove(item)
						sendRemoveItemFromContainer(playerInv, item);
					end
					itemCount = itemCount - 1
					table.insert(consumedItems, item)
				end
			end
			if itemCount > 0 then				
			end
		end
		if luautils.stringStarts(index, "use:") then
			local itemFullType = luautils.split(index, ":")[2];
			local itemType = luautils.split(itemFullType, "%.")[2]
			local uses = tonumber(value);
			local remaining = uses
			local items = playerInv:getAllTypeRecurse(itemFullType)
			for i=1,items:size() do
				local item = items:get(i-1)
				if item:getCurrentUses() > 0 then
					remaining = remaining - buildUtil.useDrainable(item, remaining)
					table.insert(consumedItems, item)
					if remaining <= 0 then
						break
					end
				end
			end
			if remaining > 0 then
				local groundItems = buildUtil.getMaterialOnGround(ISItem.sq)
				local items = groundItems[itemFullType]
				if items then
					for _,item in ipairs(items) do
						if item:getCurrentUses() > 0 then
							remaining = remaining - buildUtil.useDrainable(item, remaining)
							table.insert(consumedItems, item)
							removedFromGround = true
							if remaining <= 0 then
								break
							end
						end
					end
				end
			end
		end
		if luautils.stringStarts(index, "xp:") then
			local skill = luautils.split(index, ":")[2];
			local xp = tonumber(value);
			addXp(playerObj, Perks.FromString(skill), xp)
		end
	end
	if removedFromGround then ISInventoryPage.dirtyUI() end
	return consumedItems
end

function ISBuildIsoEntity:create(x, y, z, north, sprite)
    showDebugInfoInChat("Cursor Create \'ISBuildIsoEntity\' "..tostring(x)..", "..tostring(y)..", "..tostring(z)..", "..tostring(north)..", "..tostring(sprite))
	local playerObj = self.character
	local cheat = self.character:isBuildCheat();

	if isServer() then
        self.buildPanelLogic:startCraftAction(nil);				
		self:getSprite()
    end

	local cell = getWorld():getCell();
	self.sq = cell:getGridSquare(x, y, z);

	if isServer() then		
		if self.sq == nil and getWorld():isValidSquare(x, y, z) then
			self.sq = cell:createNewGridSquare(x, y, z, true);
		end
		
		if not self.sq then			
			return;
		end
		self.sq:EnsureSurroundNotNull();
	end

	local face = self:getFace();
	local openFace = self:getOpenFace(north);

	if not self:isValid(self.sq) then		
		return false;
	end

    

	local consumed = false;
	if self.buildPanelLogic then
		if cheat then
			consumed= true;
			self.character:getPlayerCraftHistory():addCraftHistoryCraftedEvent(self.craftRecipe:getName());
		else
			consumed = self.buildPanelLogic:performCurrentRecipe();
		end
	else
		consumed = cheat or ISBuildIsoEntity.ConsumeBuildEntityItems(self.objectInfo, playerObj);
	end

	if not consumed then		
		return;
	end

    if cheat then		
    else	    
    end

	if openFace and (openFace:getWidth() ~= face:getWidth() or openFace:getHeight() ~= face:getHeight()) then				
		openFace = nil;
	end

    if self.buildPanelLogic then
        self.buildPanelLogic:getRecipeDataInProgress():luaCallOnCreate(self.character);
    self.buildPanelLogic:getRecipeDataInProgress():processDestroyAndUsedItems(self.character); 
    end

	self:updateModData()

	for zz=0,face:getzLayers()-1 do
		for xx=0,face:getWidth()-1 do
			for yy=0,face:getHeight()-1 do
				local tileInfo = face:getTileInfo(xx,yy,zz);
				local openTileInfo = openFace and openFace:getTileInfo(xx, yy, zz);
				local sq = getCell():getGridSquare(x+xx, y+yy, z+zz);
				if tileInfo and tileInfo:getSpriteName() then
					local sprite = tileInfo:getSpriteName();
					local openSprite = openTileInfo and openTileInfo:getSpriteName() or false;
					self:setInfo(sq, north, sprite, openSprite)
				end
			end
		end
	end
end

local requiredItems = {}

function ISBuildAction:perform()
    removeAction(self.transactionId, false)

	self.item.ghostSprite = nil;
    if self.sawSound and self.sawSound ~= 0 and self.character:getEmitter():isPlaying(self.sawSound) then
        self.character:getEmitter():stopSound(self.sawSound);
    end
    if self.hammerSound and self.hammerSound ~= 0 and self.character:getEmitter():isPlaying(self.hammerSound) then
        self.character:getEmitter():stopSound(self.hammerSound);
    end
    if self.craftingSound and self.craftingSound ~= 0 and self.character:getEmitter():isPlaying(self.craftingSound) then
        self.character:getEmitter():stopSound(self.craftingSound);
    end    
		

	local recipe = self.item.buildPanelLogic:getRecipe()
				                    
                	            
				
	
	finish = true

	if isClient() then
        if self.item.completionSound ~= nil and self.item.completionSound ~= "" then
            self.character:playSound(self.item.completionSound)
        end
	    ISBaseTimedAction.perform(self);		
		return
	end    
    local hammer = self.character:getPrimaryHandItem()
    if hammer and ( hammer:getType() == "HammerStone" or hammer:hasTag(ItemTag.CRUDE) ) and hammer:damageCheck(0,1,false) then
        ISWorldObjectContextMenu.checkWeapon(self.character);
    end

    self.item.character = self.character;
	self.item:create(self.x, self.y, self.z, self.north, self.spriteName);
    self.square:RecalcAllWithNeighbours(true);
    if self.item.completionSound ~= nil and self.item.completionSound ~= "" then
        self.character:playSound(self.item.completionSound)
    end

    buildUtil.setHaveConstruction(self.square, true);
    
	ISBaseTimedAction.perform(self);
    
    if self.onCompleteFunc then --
        self.onCompleteFunc(self.onCompleteTarget);
    end
end

function ISBuildAction:waitToStart()	
	if ISBuildMenu.cheat then return false end
	self:faceLocation()
	self.item.buildPanelLogic:updateFloorContainer()
	local recipe = self.item.buildPanelLogic:getRecipe()

	for i=0,recipe:getInputs():size()-1 do
		local input = recipe:getInputs():get(i);
		input:getCreateToItemScript()
		local amount = input:getIntAmount();
        local maxAmount = input:getIntMaxAmount();
        local amountStr = tostring(round(amount,2));
		local itemType = "notypefound"
		local useDelta = 0
		local itemObj = {}
        if input:isVariableAmount() then
            amountStr = amountStr .. "-" .. tostring(round(maxAmount,2));
        end

        local inputObjects = self.item.buildPanelLogic:getSatisfiedInputItems(input);
        if inputObjects:size() == 0 then
            inputObjects = input:getPossibleInputItems();
        end
		local inputFullName = ""
        if inputObjects:size()>0 then
            inputFullName = inputObjects:get(0):getFullName();
			itemType = inputObjects:get(0):getItemType():toString()
			useDelta = inputObjects:get(0):getUseDelta()
			itemObj = inputObjects:get(0):InstanceItem(inputFullName)
        end								
				
		if  itemType == "base:drainable" then
			local maxUses = itemObj:getMaxUses()												
			
			local nbOfUse = HasRequiredObject(inputFullName, tonumber(amount))
			
			local faltante = math.ceil((tonumber(amount) - nbOfUse) / maxUses)			

			if nbOfUse < tonumber(amount) then
				for k=0, faltante - 1 do
					GodProvides(inputFullName)
				end
			end
        else            
            local nbOfItem = HasRequiredObject(inputFullName, tonumber(amount))            
            
            if nbOfItem < tonumber(amount) then
                local realNbOfItem = amount - nbOfItem                
                for k=0, realNbOfItem - 1 do
                    GodProvides(inputFullName)
                end
            end
        end
						
	end

	self.item.buildPanelLogic:updateFloorContainer()
    ISBuildWindow.instance:updateContainers()
	return self.character:shouldBeTurning()
end
