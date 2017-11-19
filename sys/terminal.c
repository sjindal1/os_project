#include <sys/defs.h>
#include <sys/kernel.h>
#include <sys/kprintf.h>
#include <sys/syscalls.h>
#include <sys/kernel.h>
#include <sys/terminal.h>

#define ystart 12
#define txmax 80
#define tymax 11
#define ncmdbuf  8
#define cmdbufsiz 128

void _termdisplayBuff();
void _termbuffCopy();

int _term_x_pos=0, _term_y_pos=0;
uint8_t _termbuf[tymax][txmax];

uint8_t cmdbuf[ncmdbuf][cmdbufsiz+1];
volatile bufdetails bufinfo[ncmdbuf];
uint8_t cur_cmd_bufy = 0, cur_cmd_xpos = 0, cur_read_bufy=0;

extern int ps2_ascii_mapping[];
extern int ps2_ascii_shift_mappings[];

void _termbuffCopy(){
    for(int i=0;i<tymax-1;i++){
        for(int j=0;j<txmax;j++){
            _termbuf[i][j] = _termbuf[i+1][j];
        }
    }
    for(int i=0; i<txmax;i++){
        _termbuf[tymax-1][i] = 0;
    }
    _term_x_pos =0;
    _term_y_pos = tymax - 1;
}

void _termdisplayBuff(){
  register uint8_t *temp1, *temp2;
  for(temp2 = (uint8_t*)PRINT_BUF_ADDRESS+160*ystart; temp2 < (uint8_t*)PRINT_BUF_ADDRESS+160*(tymax+ystart); 
  	temp2 += 2) 
  	*temp2 = 7/* white */;
  temp1 = &_termbuf[0][0];
  for(temp2 = (uint8_t*)PRINT_BUF_ADDRESS+160*ystart; temp2 < (uint8_t*)PRINT_BUF_ADDRESS+160*(tymax+ystart); 
  	temp1 += 1, temp2 += 2) 
  	*temp2 = *temp1;
}

uint64_t _termwrite(uint8_t *final_str, uint64_t size){
	//kprintf("_termwrite buf enter = %x\n", final_str);
	int array_pos = 0;
	for(;_term_y_pos<tymax;_term_y_pos++){
		for(int i=_term_x_pos;i<80;i++){
			_termbuf[_term_y_pos][i] = 0;
		}
	  for(;_term_x_pos<txmax;_term_x_pos++){
	    if(final_str[array_pos] == '\r'){
	        array_pos++;
	        _term_x_pos = 0;
	    }
	    if(final_str[array_pos] =='\n' && final_str[array_pos + 1] =='\0' && _term_y_pos == tymax-1){
	        _termbuffCopy();
	        array_pos++;
	        break;
	    }
	    if(final_str[array_pos] =='\n' && final_str[array_pos + 1] =='\0'){
	        array_pos++;
	        _term_x_pos = 0;
	        _term_y_pos++;
	        break;
	    }
	    if(final_str[array_pos] =='\n'){
	        array_pos++;
	        _term_x_pos = 0;
	        //_term_y_pos++;
	        break;
	    }else if(final_str[array_pos] == 8){		// backspace handling
	    	//kprintf("backspace %d \n", _term_x_pos);
	    	if(_term_x_pos != 0) {
	    		_term_x_pos--;
	   			_termbuf[_term_y_pos][_term_x_pos] = 0;
	    	}
	    	array_pos++;
	    	break;
	    }else if(array_pos == size){
	        break;
	    }
	    else if(final_str[array_pos] == '\0'){
	        break;
	    }else{
	        _termbuf[_term_y_pos][_term_x_pos] = final_str[array_pos];
	        array_pos++;
	    }
	  }
	  if(_term_x_pos == txmax && final_str[array_pos] == '\0'){
	    _term_y_pos++;  
	  }
	  if(_term_x_pos == txmax){
	      _term_x_pos = 0;
	  }
	  if(array_pos == size){
	  	break;
	  }
	  if(final_str[array_pos] == '\0'){
	      break;
	  }
	}
	_termdisplayBuff();
	while(final_str[array_pos] != '\0' && array_pos < size){
	  _termbuffCopy();
	  for(;_term_x_pos<txmax;_term_x_pos++){
	      if(final_str[array_pos] == '\r'){
	          array_pos++;
	          _term_x_pos = 0;
	      }
	      if(final_str[array_pos] =='\n' && final_str[array_pos + 1] =='\0'){
	          _termbuffCopy();
	          array_pos++;
	          break;
	      }
	      if(final_str[array_pos] =='\n'){
	          array_pos++;
	          break;
		    }else if(final_str[array_pos] == 8){		// backspace handling
		    	if(_term_x_pos != 0) {
		    		_term_x_pos--;
		   			_termbuf[_term_y_pos][_term_x_pos] = 0;
		    	}
		    	array_pos++;
		    	break;
	      }else if(array_pos == size){
	        break;
	    	}else if(final_str[array_pos] == '\0'){
	          break;
	      }else{
	          _termbuf[_term_y_pos][_term_x_pos] = final_str[array_pos];
	          array_pos++;
	      }
	  }
	  _termdisplayBuff();
	}

	//kprintf("_termwrite buf exit = %x\n", final_str);
	return array_pos;
}

void _termupdatecmdbuf(char str)
{
	//uint8_t cmdbuf[ncmdbuf][cmdbufsiz];
//bufdetails bufinfo[ncmdbuf];
//uint8_t cur_cmd_bufy = 0, cur_cmd_xpos = 0;
	if(bufinfo[cur_cmd_bufy].valid == 1)// || cur_cmd_xpos >= cmdbufsiz)
		return;		// skipping the key inputs

	bufinfo[cur_cmd_bufy].xpos = 0;

	if(str == '\n')
	{
		if(cur_cmd_bufy < ncmdbuf) {
			cmdbuf[cur_cmd_bufy][cur_cmd_xpos+1] = '\0';
			bufinfo[cur_cmd_bufy].size = cur_cmd_xpos;
			bufinfo[cur_cmd_bufy].valid = 1;
			kprintf("%d %d %d %s\n", cur_cmd_bufy, bufinfo[cur_cmd_bufy].size, bufinfo[cur_cmd_bufy].valid, cmdbuf[cur_cmd_bufy]);

			cur_cmd_bufy++;
			cur_cmd_xpos = 0;
		}
	}
	// backspace
	else if(str == 8)
	{
		if(cur_cmd_xpos == 0)
			return;

		cur_cmd_xpos--;
		cmdbuf[cur_cmd_bufy][cur_cmd_xpos] = 0;
	}
	else
	{
		if(cur_cmd_xpos < cmdbufsiz)
		{
			cmdbuf[cur_cmd_bufy][cur_cmd_xpos] = str;
			cur_cmd_xpos++;
		}
	}

	// handling the circular part
	if(cur_cmd_bufy == ncmdbuf){
		cur_cmd_bufy = 0;
	}

	return;
}

void _term_keypress_handle(){
	uint8_t shift_pressed = 0, ctrl_pressed = 0;
	unsigned char scan_code;
	__asm__ __volatile__ ("inb $0x60, %%al\n\t"
	                       "movb %%al, %0"
	                       :"=r"(scan_code)
	                       :
	                       :"memory");
	uint8_t str[2];
	str[1] ='\0';
	if(scan_code < 129){
	   if(scan_code == 42 || scan_code == 54){
	     shift_pressed = 1;
	   } 
	   else if(scan_code == 29){
	     ctrl_pressed = 1;
	   }else{
	     if(shift_pressed == 1){
	       str[0] =  ps2_ascii_shift_mappings[scan_code];
	     }else if(ctrl_pressed == 1){
	       str[0] =  ps2_ascii_mapping[scan_code];
	     }
	     else{
	       str[0] =  ps2_ascii_mapping[scan_code];
	     }
	     _termupdatecmdbuf(str[0]);
	     _termwrite(str, 1);
	   }
	 }else{
	   if(scan_code == 170 || scan_code == 182){
	     shift_pressed = 0;
	   } 
	   else if(scan_code == 157){
	     ctrl_pressed = 0;
	   }
	 }
}

uint64_t _termread(uint8_t * user_buf, uint64_t size){
	kprintf("inside term read, user_buf = %x, size = %d\n", user_buf, size);
	uint64_t temp = 0;
	while(1){
		volatile uint8_t flag = bufinfo[cur_read_bufy].valid;
		if(flag == 1){
			break;
		}
		temp++;
	}
	uint8_t no_bytes_copy = 0;
	if((bufinfo[cur_read_bufy].size - bufinfo[cur_read_bufy].xpos) <= size){
		no_bytes_copy = (bufinfo[cur_read_bufy].size - bufinfo[cur_read_bufy].xpos);
	}else{
		no_bytes_copy = size;
	}
	int j=0;
	for(uint8_t i = bufinfo[cur_read_bufy].xpos; i<no_bytes_copy + bufinfo[cur_read_bufy].xpos ; i++){
		user_buf[j] = cmdbuf[cur_read_bufy][i];
		j++;
	}
	if((bufinfo[cur_read_bufy].size - bufinfo[cur_read_bufy].xpos) <= size){
		bufinfo[cur_read_bufy].size = 0;
		bufinfo[cur_read_bufy].valid = 0;
		bufinfo[cur_read_bufy].xpos = 0;
		cur_read_bufy++;
	}else{
		bufinfo[cur_read_bufy].xpos = size;
	}
	kprintf("returning from read user_buf - %s",user_buf);
	return no_bytes_copy;
}