   M S B A M L   `   `   `  ����D  @usbip-gui, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null%clr-namespace:usbip_gui	usbip_gui  5   	       usbip_gui.MainWindow   5   	   \ XPresentationFramework, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35R NWindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35W SPresentationCore, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35D 9http://schemas.microsoft.com/winfx/2006/xaml/presentation    5   	   R NSystem.Xaml, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b77a5c561934e089:x,http://schemas.microsoft.com/winfx/2006/xaml     5   	   @d2http://schemas.microsoft.com/expression/blend/2008     5   	   Jmc;http://schemas.openxmlformats.org/markup-compatibility/2006     5   	   )localclr-namespace:usbip_gui     5   	     � Title$  USBIP��5   	   $	��450��6   $��862.524��6$   .��5
      � .+�5   
   �� -   5         ��Name$ TB_ServerIP��5      $
��Left=�6'   $��23��6B   $��	30,46,0,0q�6N    �� TextWrapping$
 Wrap=�6a   $��192.168.0.41��6u   $	��Top=�6�   $	��120��6�   �� 5   
   -   5         ��Name$ Btn_GetBoundDevices��5      $��Get bound devices��6.   $
��Left=�6J   $��
174,49,0,0q�6e   $	��Top=�6y   $	��128��6�   6�   &   System.Windows.Controls.DataGrid  5   
   -   5          Name$ 	dataGrid1��5         AutoGenerateColumns .  $
��Left=�6D   $	��117��6_   $��	30,98,0,0q�6l   $	��Top=�6   $	��806��6�      Columns 5      0  *System.Windows.Controls.DataGridTextColumn  5      ,  &System.Windows.Controls.DataGridColumn   Header$ Id��6%   1  +System.Windows.Controls.DataGridBoundColumn   Binding 61   �� *Id+6J     5      $ Manufacturer��6%    6;   �� *Manufacturer+6^     5      $ Device Name��6%    6:   �� *Name+6U     5      $ Device Bound��6%    6;   �� *Bound+6W     5      $ Remote Bus ID��6%    6<   �� *BusId+6X     5      $ 	Remote IP��6%    68   �� *	IpAddress+6X     5      $ Attached��6%    67   �� *
Attached+6V   4  .System.Windows.Controls.DataGridTemplateColumn  5      $ Attach/Detach��6)   	   CellTemplate	 5      �� 5      .R�5      �� $  Btn_ChangeDeviceBindStatus��6%   -   5        .��5   r   Attach6z   5      5      5      �� 5   
   -   5        $ 	TB_Output��5      $
��Left=�6%   $	��154��6@   $��
56,241,0,0q�6M   $
 Wrap=�6a   $��TextBox��6u   $	��Top=�6�   $	��634��6�   "
 �� VerticalScrollBarVisibility$

 Auto=�6�   �� 5    
   -   5        $ Btn_GetUnboundDevices��5       $��Get unbound devices��60   $
��Left=�6N   $��
322,49,0,0q�6i   $	��Top=�6}   $	��128��6�   6�   �� 5!   
   -   5        $ Btn_GetRemoteDevices��5!      $��Get Remote Devices��60   $
��Left=�6M   $��
486,49,0,0q�6h   $	��Top=�6|   $	��125��6�   6�   5"      5$      