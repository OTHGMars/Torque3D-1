//-----------------------------------------------------------------------------
// This script adds buttons to the main menu and should only execute after all
// dependencies have been exec'ed.
//-----------------------------------------------------------------------------

if (isObject(MainMenuGui))
{
   %testBtn = new GuiButtonCtrl() {
      text = "Web Demo";
      groupNum = "-1";
      buttonType = "PushButton";
      useMouseEvents = "0";
      position = "0 0";
      extent = "200 40";
      minExtent = "8 8";
      horizSizing = "right";
      vertSizing = "bottom";
      profile = "GuiBlankMenuButtonProfile";
      visible = "1";
      active = "1";
      command = "webCtrlDemo.toggleDemo();";
      tooltipProfile = "GuiToolTipProfile";
      tooltip = "Web demo window (Try it with a gamepad...)";
      isContainer = "0";
      canSave = "0";
      canSaveDynamicFields = "0";
   };

   %browserBtn = new GuiButtonCtrl() {
      text = "Web Browser";
      groupNum = "-1";
      buttonType = "PushButton";
      useMouseEvents = "0";
      position = "0 0";
      extent = "200 40";
      minExtent = "8 8";
      horizSizing = "right";
      vertSizing = "bottom";
      profile = "GuiBlankMenuButtonProfile";
      visible = "1";
      active = "1";
      command = "webCtrlDemo.toggleBrowser();";
      tooltipProfile = "GuiToolTipProfile";
      tooltip = "Web browser window";
      isContainer = "0";
      canSave = "0";
      canSaveDynamicFields = "0";
   };

   %urlBtn = new GuiButtonCtrl() {
      text = "URL Request";
      groupNum = "-1";
      buttonType = "PushButton";
      useMouseEvents = "0";
      position = "0 0";
      extent = "200 40";
      minExtent = "8 8";
      horizSizing = "right";
      vertSizing = "bottom";
      profile = "GuiBlankMenuButtonProfile";
      visible = "1";
      active = "1";
      command = "webCtrlDemo.toggleURLTester();";
      tooltipProfile = "GuiToolTipProfile";
      tooltip = "URL Request test gui";
      isContainer = "0";
      canSave = "0";
      canSaveDynamicFields = "0";
   };

   if (!isObject(MMTestContainer))
   {
      new GuiDynamicCtrlArrayControl(MMTestContainer) {
         colCount = "0";
         colSize = "200";
         rowCount = "0";
         rowSize = "40";
         rowSpacing = "2";
         colSpacing = "0";
         frozen = "0";
         autoCellSize = "0";
         fillRowFirst = "1";
         dynamicSize = "1";
         padding = "0 0 0 0";
         position = "0 0";
         extent = "200 40";
         minExtent = "8 2";
         horizSizing = "right";
         vertSizing = "bottom";
         profile = "GuiDefaultProfile";
         visible = "1";
         active = "1";
         tooltipProfile = "GuiToolTipProfile";
         hovertime = "1000";
         isContainer = "1";
         canSave = "0";
         canSaveDynamicFields = "0";
      };
      MainMenuGui.add(MMTestContainer);
   }

   MMTestContainer.add(%testBtn);
   MMTestContainer.add(%browserBtn);
   MMTestContainer.add(%urlBtn);
}

// Convert our local html path
WebDemoControl.StartURL = "file:///" @ getMainDotCsDir() @ "/data/webCtrlDemo/html/demoTestGui.html";