//kernel head
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/circ_buf.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <asm/io.h>
#include <asm/irq.h>


//local head
#include "../inc/rctty.h"
#include "../inc/rcuart.h"
#include "../inc/serial_xr16m752.h"
#include "../inc/rctty_flip.h"

//#define  XR16V544_VA_BASE_ADDRESS    
 void*   XR16V544_VA_BASE_ADDRESS;
#define  XR16V544_PHY_BASE_ADDRESS    (0xe0004900-0x300)          //duart3  寄存器地址
#define  XR16V544_PORT_ADDRESS_STEP  (0x100)

#define  CLK_FRE  133334000   //crystall frequency * pll
                //12288000      

#define  B9600_H ((CLK_FRE/16/9600)>>8)
#define  B9600_L ((CLK_FRE/16/9600)&0xFF)

#define  SERIAL_XR16_FIFO     16 


/*
 * When a break, frame error, or parity error happens, 
 * these codes are stuffed into the flags buffer.
 */
#define TTY_NORMAL  0
#define TTY_BREAK   1
#define TTY_FRAME   2
#define TTY_PARITY  3
#define TTY_OVERRUN 4


#define XR_WRITEB(val,addr)  writeb(val,addr)
#define XR_READB(addr)       readb(addr)


/*interrpt handle name */

extern char *rcuartname[UART_NR];
extern struct rcuart_port *pxr_ports[UART_NR];


/*******************************************************************************
*描述 关闭发送中断
*
*输入 uart端口描述
*
*输出 无
*
*返回   
*/

void xr_stop_tx_duart(struct rcuart_port *uap)
{
	while(!(XR_READB(uap->membase + 0x5)&UART_LSR_TEMT));  //等待发送寄存器空 
    XR_WRITEB(0,uap->membase + UART_MCR);        			

    uap->im &= ~UART_IER_THRI;
    XR_WRITEB(uap->im, uap->membase + UART_IER);
   
    return; 
}

/*******************************************************************************
*描述 打开发送中断
*
*输入 uart端口描述
*
*输出 无
*
*返回 
*  
*/
void xr_start_tx_duart(struct rcuart_port *uap)
{
//位置流控，发送使能
	XR_WRITEB(UART_MCR_RTS,uap->membase + UART_MCR);        

	uap->im |= UART_IER_THRI;
    XR_WRITEB(uap->im, uap->membase + UART_IER);

    return; 
}


/*******************************************************************************
*描述 UART接收字符处理
*
*输入 uart端口描述，当前LSR掩码
*
*输出 无
*
*返回 
*  
*/
static  void xr_rx_chars_duart(struct rcuart_port *uap, unsigned char *status)
{
  struct rctty_struct *tty = uap->tty;    
  
  int max_count = 256;
    unsigned char ch, lsr;
  char flag;

    lsr =  *status;
        
  do {
      /* The following is not allowed by the tty layer and
       unsafe. It should be fixed ASAP */
         if (unlikely(tty->flip.count >= TTY_FLIPBUF_SIZE)) 
          {
              
               tty_flip_buffer_push(tty);  
                return;
                 /* If this failed then we will throw away the
               bytes but must do so to clear interrupts */
          }
    
         ch = XR_READB(uap->membase+UART_RX);          
         flag = TTY_NORMAL;
         uap->icount.rx++;
		 //printk("%x\n",ch);

       if (unlikely(lsr & (UART_LSR_BI | UART_LSR_PE |UART_LSR_FE | UART_LSR_OE))) 
       {
           /*
            * For statistics only
            */
           if (lsr & UART_LSR_BI) 
           {
              lsr &= ~(UART_LSR_FE | UART_LSR_PE);
              uap->icount.brk++;
              /*
               * We do the SysRQ and SAK checking
               * here because otherwise the break
               * may get masked by ignore_status_mask
               * or read_status_mask.
               */
               flag = TTY_BREAK;
            } 
            else if (lsr & UART_LSR_PE)
            {
                uap->icount.parity++;  
                flag = TTY_PARITY;
            }
           else if (lsr & UART_LSR_FE)
           {
                uap->icount.frame++;  
                flag = TTY_FRAME;
            }
            if (lsr & UART_LSR_OE)
            { 
                uap->icount.overrun++;
            }            
            
       }

       
    tty_insert_flip_char(tty, ch, flag);
    
    #if 0
         
     /* Overrun is special, since it's reported
      * immediately, and doesn't affect the current
      * character.    */
      // insert 0 is meanless,lost will be show in statics  
        if ( (lsr & UART_LSR_OE) && (tty->flip.count < TTY_FLIPBUF_SIZE) ) 
        {
          tty_insert_flip_char(tty, 0, TTY_OVERRUN);
        }
        
    #endif        
      
      lsr =  XR_READB(uap->membase+UART_LSR);
    } 
    while ((lsr & UART_LSR_DR) && (max_count-- > 0));
  
    tty_flip_buffer_push(tty);  
    *status = lsr;           
           
    return;
}


/*******************************************************************************
*描述 UART发送字符处理
*
*输入 uart端口描述
*
*输出 无
*
*返回 
*  
*/
static void xr_tx_chars_duart(struct rcuart_port *uap,unsigned int count )
{

    struct circ_buf *xmit = &uap->tty->xmit;
    
  //int count = SERIAL_XR16_FIFO;
    
  if ( uart_circ_empty(xmit) ) 
  {
      //printk(KERN_INFO "no tx\n"); 
      xr_stop_tx_duart(uap);
      return;        
  }    

	do 
	{
		//printk("%x\n",xmit->buf[xmit->tail]);
		//printk("FIFO %x\n",XR_READB(uap->membase + 0x2));
		//printk("THE  %x\n",XR_READB(uap->membase + 0x5));
		//while(!(XR_READB(uap->membase + 0x5)&UART_LSR_THRE)); //这里必须等待fifo可写，才能继续写
		XR_WRITEB(xmit->buf[xmit->tail], uap->membase + UART_TX);
		xmit->tail = (xmit->tail + 1) & (PAGE_SIZE - 1);
		uap->icount.tx++;
		//
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	//up(&uap->tty->tx_over);
	//wakeup write
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
	{
		tasklet_schedule(&uap->tlet);
	}

	if (uart_circ_empty(xmit))
	{
		xr_stop_tx_duart(uap);
	}
	   
	return;
}


/*******************************************************************************
*描述 UART中断函数handle
*
*输入 标准内核接口
*
*输出 无
*
*返回 
*  
*/
static irqreturn_t xr_int_duart(int irq, void *dev_id)
{        
    struct rcuart_port *uap = dev_id;
        
    unsigned char iir;    
    unsigned char status;
    
    iir =  XR_READB(uap->membase+UART_IIR); 
    if (!(iir & UART_IIR_NO_INT))
    {
		 //printk("start int uart %x\n",iir);
		 status = XR_READB(uap->membase+UART_LSR);
         if (status & UART_LSR_DR)//
         {
			//printk(KERN_INFO "read\n");	
            xr_rx_chars_duart(uap,&status);
         }

//		 if(XR_READB(uap->membase + UART_MCR) & UART_MCR_RTS)
		 {
         	 iir = iir&0x3F;
			 if ( UART_IIR_THRI == iir )  
	         {
				  //printk(KERN_INFO "send 16\n");	
	              xr_tx_chars_duart(uap,8);
			 }	         
/*			 if(status & UART_LSR_THRE)  //在定时模式下查询FIFO是否为空
	         {
				  //printk(KERN_INFO "send 64\n");	
			 	  xr_tx_chars_duart(uap,16);
	         }
	         else if ( UART_IIR_THRI == iir )  
	         {
				  //printk(KERN_INFO "send 16\n");	
	              xr_tx_chars_duart(uap,16);
	         }*/
		 }

        return IRQ_HANDLED; 
    }
	else
    {                
        return IRQ_NONE;
    }
	return IRQ_HANDLED; 
}



/*******************************************************************************
*描述 UART设置break信号状态
*
*输入 uart端口描述,break控制信号
*
*输出 无
*
*返回 
*  
*/
void xr_break_ctl_duart(struct rcuart_port *uap, int break_state)
{
   unsigned char lcr = XR_READB(uap->membase+UART_LCR);

    if ( break_state )
   { 
        lcr |= UART_LCR_SBC; 
    }
   else
   {
        lcr &= ~UART_LCR_SBC;
   }
    
    XR_WRITEB(lcr, uap->membase + UART_LCR);
    return;
}

/*******************************************************************************
*描述 UART初始化
*
*输入 uart端口描述符
*
*输出 无
*
*返回 
*  
*/
int xr_startup_duart(struct rcuart_port *uap)
{

    int retval;
    int i = uap->line;

  /*
   * Allocate the IRQ
   */
  uap->irq = 16;
  retval = request_irq(uap->irq, xr_int_duart, IRQF_SHARED, rcuartname[i], uap);
  //printk("register request irq  %d\n",uap->irq);       
  if (retval)
  {
        printk("Error request irq \n");       
        return retval;
  }

        //disable all interrupt
        XR_WRITEB(0,uap->membase + UART_LCR);//选择常规寄存器
        XR_WRITEB(0,uap->membase + UART_IER);//清零中断使能
        
        //clear interrupt        
        XR_READB(uap->membase+UART_LSR);
        XR_READB(uap->membase+UART_MSR);
        XR_READB(uap->membase+UART_IIR);
		
        //reset clear enable FIFO, rec 1byte interrupt
        XR_WRITEB(UART_FCR_ENABLE_FIFO|UART_FCR_CLEAR_RCVR|UART_FCR_CLEAR_XMIT|UART_FCR_TFIFO_LEVEL_4B,
        uap->membase + UART_FCR);
		
        //default baud 9600
        XR_WRITEB(UART_LCR_DLAB,uap->membase + UART_LCR);//切换到分频器模式
        XR_WRITEB(B9600_L,uap->membase + UART_DLL);
        XR_WRITEB(B9600_H,uap->membase + UART_DLM);

		//enable special register
        //XR_WRITEB(0xBF,uap->membase + UART_LCR);
        //XR_WRITEB(UART_EFR_ENABLESB,uap->membase + UART_EFR);

		//8bit 1stop no parity
        XR_WRITEB(UART_LCR_WLEN8,uap->membase + UART_LCR); //设置协议模式，切回常态
        //reset clear enable FIFO, rec 1byte interrupt 
        // ONLY after EFR OK , T_FIFO can be configured
        // so we doit again
        XR_WRITEB(UART_FCR_ENABLE_FIFO|UART_FCR_CLEAR_RCVR|UART_FCR_CLEAR_XMIT|UART_FCR_TFIFO_LEVEL_14B,
        uap->membase + UART_FCR);
        /*
         * Finally, enable interrupts.  Note: Modem status interrupts
         * are set via set_termios(), which will be occurring imminently
         * anyway, so we don't enable them here.
         */
        uap->im = UART_IER_RLSI | UART_IER_RDI;
//        uap->im = 0x0;   //can t use interrapt
        XR_WRITEB(uap->im,uap->membase + UART_IER);        

		// loop back mode for test        
//        XR_WRITEB(UART_MCR_LOOP,uap->membase + UART_MCR);        
        // And clear the interrupt registers again for luck.

		//485模式需要开流控 发送时置位
//        XR_WRITEB(UART_MCR_RTS,uap->membase + UART_MCR);        
        XR_WRITEB(0,uap->membase + UART_MCR);        
     

        XR_READB(uap->membase+UART_LSR);
        XR_READB(uap->membase+UART_RX);
        XR_READB(uap->membase+UART_IIR);
        XR_READB(uap->membase+UART_MSR);
/*
	unsigned char h,l,old,tt,ret;
	old = readb(uap->membase+0x03);
	writeb(0x80,uap->membase + 0x03);//切换到分频器模式
	l = readb(uap->membase+0);
	h = readb(uap->membase+1);
	tt = readb(uap->membase+2);
	writeb(old,uap->membase + 0x03);//切回

	printk("h : %x\n",h);
	printk("l : %x\n",l);
	printk("tt : %x\n",tt);

	for(ret=0;ret<8;ret++)
	{
		printk("%d : %x\n",ret,readb(uap->membase+ret));
	}*/

        
  return 0;
}


/*******************************************************************************
*描述 关闭UART
*
*输入 uart端口描述符
*
*输出 无
*
*返回 
*  
*/
void xr_shutdown_duart(struct rcuart_port *uap)
{
    unsigned char lcr;
    /*
   * disable all interrupts
   */
    xr_stop_tx_duart(uap);    
    uap->im = 0;    
    XR_WRITEB(uap->im,uap->membase + UART_IER);
    /*
   * Free the interrupt
   */
    free_irq(uap->irq, uap);
     /*
      *  Disable  break condition and FIFOs
      */
    XR_WRITEB(UART_FCR_CLEAR_RCVR|UART_FCR_CLEAR_XMIT,
        uap->membase + UART_FCR);

    lcr = XR_READB(uap->membase+UART_LCR);
    lcr = lcr & ~UART_LCR_SBC;
    XR_WRITEB(lcr,uap->membase + UART_LCR); 
}






/*******************************************************************************
*描述 使能auto485
*
*输入 uart端口描述
*
*输出 无
*
*返回   
*/

void xr_enable_485_duart(struct rcuart_port *uap)
{    
    //lcr enable write DLD
   XR_WRITEB(UART_MCR_RTS,uap->membase + UART_MCR);        
   return;
}



/*******************************************************************************
*描述 关闭auto485
*
*输入 uart端口描述
*
*输出 无
*
*返回   
*/

void xr_disable_485_duart(struct rcuart_port *uap)
{
    //等待数据发送完成
	while(!(XR_READB(uap->membase + 0x5)&UART_LSR_TEMT)); 
	//拉低RTS
	XR_WRITEB(0,uap->membase + UART_MCR);        
 
    return;
}

/*******************************************************************************
*描述 设置UART端口参数
*
*输入 uart端口描述符，端口参数
*
*输出 无
*
*返回 
*  
*/
void xr_set_termios_duart(struct rcuart_port *uap, struct rctty_base_param_s * ptty_base_param)
{
    unsigned int uiBaud = ptty_base_param->baud;
    unsigned int uiQuot = CLK_FRE/16/uiBaud ;
    unsigned char ucDlm,ucDll,ucDld;
    unsigned char ucLcr = 0;
    unsigned char Dld;

    ucDlm = uiQuot>>8;
    ucDll = uiQuot&0xFF;
    ucDld = (CLK_FRE/uiBaud) - uiQuot*16;

     //set baud 
      XR_WRITEB(UART_LCR_DLAB,uap->membase + UART_LCR);
      XR_WRITEB(ucDll,uap->membase + UART_DLL);
      XR_WRITEB(ucDlm,uap->membase + UART_DLM);
//      Dld = XR_READB(uap->membase+UART_DLD);
//      Dld = (Dld&0xF0)|(ucDld&0x0F);
//      XR_WRITEB(Dld,uap->membase + UART_DLD);
     //length
      switch(ptty_base_param->datalen)
      {
        case 5: 
            ucLcr = UART_LCR_WLEN5;
            break;
        case 6: 
            ucLcr = UART_LCR_WLEN6;
            break;
        case 7: 
            ucLcr = UART_LCR_WLEN7;
            break;
        case 8: 
        default:
            ucLcr = UART_LCR_WLEN8;
            break;
       }
     //check
     switch(ptty_base_param->check)
     {
        case 0: //NO parity
            
            break;
        case 1: //odd check 
             ucLcr |= UART_LCR_PARITY;
             break;
        case 2: //even
             ucLcr |= (UART_LCR_PARITY|UART_LCR_EPAR);
             break;
        case 3: //mark
             ucLcr |= (UART_LCR_PARITY|UART_LCR_SPAR);
             break;
        case 4: //space
             ucLcr |= (UART_LCR_PARITY|UART_LCR_SPAR|UART_LCR_EPAR);
             break;
             
        default:
            break;        
      }
     //stopbit
     switch(ptty_base_param->stopbit)
     {
        case 1:
          break;
          
        case 2:
            ucLcr |= UART_LCR_STOP;
            break;

        default:
            break;
      }   
     
    XR_WRITEB(ucLcr,uap->membase + UART_LCR);
	
    return;
}

/*******************************************************************************
*描述 UART物理参数设置，资源分配
*
*输入 无
*
*输出 无
*
*返回 
*  
*/
int xr_probe_duart(unsigned int irq)
{
    struct rcuart_port *uap;
    int i, ret = 0;

  for (i = 3; i < UART_NR; i++)
  {
        
      uap = kmalloc(sizeof(struct rcuart_port), GFP_KERNEL);
      if (uap == NULL)
      {
            ret = -ENOMEM;
            printk(KERN_WARNING"error uap Init \n");
            goto out;
      }  

      memset(uap, 0, sizeof(struct rcuart_port));
      
      XR16V544_VA_BASE_ADDRESS = ioremap(XR16V544_PHY_BASE_ADDRESS,0x1000);  //地址映射，获取虚拟地址
               
		uap->mapbase =         XR16V544_PHY_BASE_ADDRESS + (i)*XR16V544_PORT_ADDRESS_STEP;
		uap->membase = (void*)(XR16V544_VA_BASE_ADDRESS  + (i)*XR16V544_PORT_ADDRESS_STEP);     
		uap->irq = irq;
		uap->line = i;

		uap->uart_type      = 0;   //for duart
		uap->xr_break_ctl   = xr_break_ctl_duart; 
		uap->xr_set_termios = xr_set_termios_duart;
		uap->xr_shutdown    = xr_shutdown_duart;
		uap->xr_startup     = xr_startup_duart;
		uap->xr_start_tx    = xr_start_tx_duart;
		uap->xr_enable_485  = xr_enable_485_duart;
		uap->xr_disable_485  = xr_disable_485_duart;

	  //uap->xr_stop_tx     = xr_stop_tx_duart;
     
        
      //printk(KERN_ALERT"ssr:%d port\n",i);

      pxr_ports[i] = uap;

  }

       
out:
  return ret;
    
}



/*******************************************************************************
*描述 UART物理资源释放
*
*输入 无
*
*输出 无
*
*返回 
*  
*/
void xr_unprobe_duart(void)
{
    int i = 0;
    struct rcuart_port *uap;
    
    

    for (i = 3; i < UART_NR; i++)
    {
         uap = pxr_ports[i];
         pxr_ports[i] = NULL;
         kfree(uap);
    }
    
    iounmap(XR16V544_VA_BASE_ADDRESS);
   
    return;
}




