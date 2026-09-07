function ISWidgetTooltipOutput:updateScriptValues(_table)
    local index = 0;
    if _table.script:getResourceType()==ResourceType.Item then
        if _table.cycleIcons then
            local playerIndex = self.player:getPlayerNum();
            index = UIManager.getSyncedIconIndex(playerIndex, _table.outputObjects:size());
        end
        local item = _table.outputObjects:get(index);
        _table.iconTexture = item:getNormalTexture();
		_table.iconColor.r = item:getR();
        _table.iconColor.g = item:getG();
        _table.iconColor.b = item:getB();
        _table.iconText = item:getDisplayName() .. " ID : " .. item:getFullName();
    elseif _table.script:getResourceType()==ResourceType.Fluid then
        if _table.cycleIcons then
            local playerIndex = self.player:getPlayerNum();
            index = UIManager.getSyncedIconIndex(playerIndex, _table.outputObjects:size());
        end
        local fluid = _table.outputObjects:get(index);
        local c = fluid:getColor();
        _table.iconColor.r = c:getRedFloat();
        _table.iconColor.g = c:getGreenFloat();
        _table.iconColor.b = c:getBlueFloat();
        _table.iconText = fluid:getDisplayName() .. " ID : " .. fluid:getFullName();
    elseif _table.script:getResourceType()==ResourceType.Energy then
        if _table.cycleIcons then
            local playerIndex = self.player:getPlayerNum();
            index = UIManager.getSyncedIconIndex(playerIndex, _table.outputObjects:size());
        end
        local energy = _table.outputObjects:get(index);
        _table.iconTexture = energy:getIconTexture();
        local c = energy:getColor();
        _table.iconColor.r = c:getRedFloat();
        _table.iconColor.g = c:getGreenFloat();
        _table.iconColor.b = c:getBlueFloat();
        _table.iconText = energy:getDisplayName() .. " ID : " .. energy:getFullName();
    end

    if _table.icon then
        _table.icon.texture = _table.iconTexture;
        _table.icon.backgroundColor.r = _table.iconColor.r;
        _table.icon.backgroundColor.g = _table.iconColor.g;
        _table.icon.backgroundColor.b = _table.iconColor.b;

        if _table.iconText then
            _table.icon:setMouseOverText(_table.iconText);
        end
    end
end

function ISWidgetTooltipOutput:updateValues()
    if self.primary then
        self:updateScriptValues(self.primary);
    end

    if self.secondary then
        if self.secondary.isCreate then
            self:updateScriptValues(self.secondary);
        else
            ISWidgetInput.updateScriptValues(self, self.secondary);
        end
    end

    if self.logic and self.interactiveMode and self.outputScript:getResourceType()==ResourceType.Item then
        if self.logic:isManualSelectInputs() then
            local outputMapper = self.outputScript:getOutputMapper();
            local outputItem = outputMapper:getOutputItem(self.logic:getRecipeData(), true);
            if outputItem and outputItem:getNormalTexture() then
                self.primary.icon.texture = outputItem:getNormalTexture();
                self.primary.icon:setMouseOverText(outputItem:getDisplayName() .. " ID : " .. outputItem:getFullName());
            end
        end
    end
end

function ISWidgetOutput:updateValues()
    if self.primary then
        self:updateScriptValues(self.primary);
    end

    if self.secondary then
        if self.secondary.isCreate then
            self:updateScriptValues(self.secondary);
        else
            ISWidgetInput.updateScriptValues(self, self.secondary);
        end
    end

    if self.logic and self.interactiveMode and self.outputScript:getResourceType()==ResourceType.Item then
        if self.logic:isManualSelectInputs() then
            local outputMapper = self.outputScript:getOutputMapper();
            local outputItem = outputMapper:getOutputItem(self.logic:getRecipeData(), true);
            if outputItem and outputItem:getNormalTexture() then
                self.primary.icon.texture = outputItem:getNormalTexture();
                self.primary.icon:setMouseOverText(outputItem:getDisplayName() .. " ID : " .. outputItem:getFullName());
            end
            if outputItem and self.primary.itemNameLabel then
                self.primary.iconText = outputItem:getDisplayName();
                self.editedLabels = true;
            end
        end

        if self.editedLabels then
            self.editedLabels = false;
            self:calculateLayout(self.width,self.height);
        end
    end
end

function ISWidgetOutput:updateScriptValues(_table)
    local index = 0;
    local oldIconText = _table.iconText;
    local oldQtyText = _table.amountStr;
    if _table.script:getResourceType()==ResourceType.Item then
        if _table.cycleIcons then
            local playerIndex = self.player:getPlayerNum();
            index = UIManager.getSyncedIconIndex(playerIndex, _table.outputObjects:size());
        end
        local item = _table.outputObjects:get(index);
        _table.iconTexture = item:getNormalTexture();
		_table.iconColor.r = item:getR();
        _table.iconColor.g = item:getG();
        _table.iconColor.b = item:getB();
        _table.iconText = item:getDisplayName() .. " ID : " .. item:getFullName();
        _table.inputFullName = item:getFullName();
    elseif _table.script:getResourceType()==ResourceType.Fluid then
        if _table.cycleIcons then
            local playerIndex = self.player:getPlayerNum();
            index = UIManager.getSyncedIconIndex(playerIndex, _table.outputObjects:size());
        end
        local fluid = _table.outputObjects:get(index);
        local c = fluid:getColor();
        _table.iconColor.r = c:getRedFloat();
        _table.iconColor.g = c:getGreenFloat();
        _table.iconColor.b = c:getBlueFloat();
        _table.iconText = fluid:getDisplayName() .. " ID : " .. fluid:getFullName();
        _table.inputFullName = fluid:getFullName();
    elseif _table.script:getResourceType()==ResourceType.Energy then
        if _table.cycleIcons then
            local playerIndex = self.player:getPlayerNum();
            index = UIManager.getSyncedIconIndex(playerIndex, _table.outputObjects:size());
        end
        local energy = _table.outputObjects:get(index);
        _table.iconTexture = energy:getIconTexture();
        local c = energy:getColor();
        _table.iconColor.r = c:getRedFloat();
        _table.iconColor.g = c:getGreenFloat();
        _table.iconColor.b = c:getBlueFloat();
        _table.iconText = energy:getDisplayName() .. " ID : " .. energy:getFullName();
        _table.inputFullName = energy:getFullName();
    end

    local variableInputRatio = _table.script:isVariableAmount() and self.logic:getVariableInputRatio() or 1;
    if variableInputRatio ~= _table.variableInputRatio then
        _table.variableInputRatio = variableInputRatio;
        local outputAmount = math.min(_table.amount * _table.variableInputRatio, _table.maxAmount);
        _table.amountStr = tostring(round(outputAmount,2));
    end

    if _table.icon then
        _table.icon.texture = _table.iconTexture;
        _table.icon.backgroundColor.r = _table.iconColor.r;
        _table.icon.backgroundColor.g = _table.iconColor.g;
        _table.icon.backgroundColor.b = _table.iconColor.b;

        if _table.iconText then
            _table.icon:setMouseOverText(_table.iconText);
        end
    end

    if (_table.itemNameLabel and oldIconText ~= _table.iconText) or
        (_table.label and oldQtyText ~= _table.amountStr) then
        self.editedLabels = true;
    end
end