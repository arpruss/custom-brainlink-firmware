// While the project as a whole is licensed under the CC3.0 sharealike license, you are free to use the code
// in this file under the BSD 3-clause-license as well.

package mobi.omegacentauri.brainlinkfirmwareuploader;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Arrays;

import android.util.Log;

public class Butterfly {
	private DataLink dataLink;
	private int flashSizeBytes;
	private int flashPageSizeWords;
	private int flashPageSizeBytes;
	private static final int TIMEOUT = 8000;
	private int RETRIES = 3;

	public Butterfly(DataLink dataLink, int flashSizeBytes, int flashPageSizeWords) {
		this.dataLink = dataLink;
		this.flashSizeBytes = flashSizeBytes; // must be a multiple of 2 * pageSizeWords
		this.flashPageSizeWords = flashPageSizeWords;
		this.flashPageSizeBytes = 2 * flashPageSizeWords;
	}
	
	public boolean start() {
		dataLink.clearBuffer();
		dataLink.transmit('P');
		return succeeded();
	}
	
	public boolean stop() {
		dataLink.clearBuffer();
		dataLink.transmit('L');
		return succeeded();
	}
	
	private boolean succeeded() {
		byte[] success = new byte[1];
		return dataLink.readBytes(success, TIMEOUT) && success[0] == 13;
	}

	public void sleep(int ms) {
		try {
			Thread.sleep(ms);
		} catch (InterruptedException e) {
		}
	}
	
	byte[] readFlashPage(int address) {
		dataLink.clearBuffer();
		byte[] block = new byte[flashPageSizeBytes];
		setAddress(address/2);
		dataLink.transmit('g', 0xFF&(flashPageSizeBytes>>8), flashPageSizeBytes&0xFF, 'F');
		if (dataLink.readBytes(block, TIMEOUT)) {
			return block;
		}
		else {
			return null;
		}
	}
	
	public boolean setAddress(int address) {
		dataLink.clearBuffer();
		dataLink.transmit('A', 0xFF&(address>>8), 0xFF&address);
		return succeeded();
	}
	
	public static void dump(byte[] block) {
		String out = "";
		for (byte b: block) {
			out += String.format("%02X ", 0xFF&(int)b);
		}
		System.out.println(out);
	}
	
//	byte[] readFlash() {
//		byte[] flash = new byte[257];
//		dataLink.clearBuffer();
//		dataLink.transmit('g', 1, 0, 'F');
//		
//		if (dataLink.readBytes(flash, 10000)) {
//			dump(flash);
//			return flash;
//		}
//		else {
//			return null;
//		}
//	}
	
	byte[] readFlash() {
		byte[] flash = new byte[flashSizeBytes];
		for (int i = 0 ; i < flashSizeBytes; i += flashPageSizeBytes) {
			int j;
			for (j = 0 ; j < RETRIES ; j++) {
				byte[] block = readFlashPage(i);
				if (block != null) {
					System.arraycopy(block, 0, flash, i, flashPageSizeWords * 2);
					break;
				}
			}
			if (j >= RETRIES) {
				return null;
			}
		}
		return flash;
	}
	
	private boolean emptyPage(byte[] flash, int address) {
		for(int i=0 ; i < flashPageSizeBytes ; i++)
			if (flash[address + i] != (byte)0xFF)
				return false;

		//Log.v("BLFW", "empty page at "+Integer.toHexString(address));
		return true;
	}
	
	boolean writeFlash(byte[] flash) {
		if (flash.length != flashSizeBytes) 
			return false;
		
		eraseChip();  
		
		for (int address = 0 ; address < flashSizeBytes ; address += flashPageSizeBytes) {
			int j;
			
			for (j = 0 ; j < RETRIES ; j++) {
				if (emptyPage(flash, address) || writeFlashPageChecked(flash, address)) {
					break;
				}
			}
			if (j >= RETRIES) {
				return false;
			}
			
		}
		
		return true; 
	}
	
	public boolean eraseChip() {
		dataLink.clearBuffer();
		dataLink.transmit('e');
		return succeeded();
	}

	private boolean writeFlashPageChecked(byte[] flash, int address) {
		byte[] out = new byte[flashPageSizeBytes];
		System.arraycopy(flash, address, out, 0, flashPageSizeBytes);
		setAddress(address/2);
		dataLink.clearBuffer();
		dataLink.transmit('B', 0xFF&(flashPageSizeBytes>>8), flashPageSizeBytes&0xFF, 'F');
		dataLink.transmit(out);
		if (!succeeded())
			return false;
		byte[] readBack = readFlashPage(address);
		if (readBack == null)
			return false;
//		if (address == 6400) {
//			Log.v("BLFW", "original");
//			for (int i = 0 ; i < out.length ; i += 16) {
//				String outs = "";
//				for (int j = 0 ; i + j < out.length && j < 16 ; j++)
//					outs += String.format("%02x ", out[i+j]);
//				Log.v("BLFW", outs);
//			}
//			Log.v("BLFW", "new");
//			for (int i = 0 ; i < readBack.length ; i += 16) {
//				String outs = "";
//				for (int j = 0 ; i + j < readBack.length && j < 16 ; j++)
//					outs += String.format("%02x ", readBack[i+j]);
//				Log.v("BLFW", outs);
//			}
//		}
		return Arrays.equals(readBack, out);
	}
	
	public byte[] getFlashFromHex(InputStream is) {
		byte[] out = new byte[flashSizeBytes];
		Arrays.fill(out, (byte)0xFF);
		byte[] returnValue = null;
		BufferedReader r = new BufferedReader(new InputStreamReader(is));
		
		String line;
		try {
			while (null != (line = r.readLine())) {
				if (!line.startsWith(":"))
					continue;
				if (line.startsWith(":00000001FF")) {
					returnValue = out;
					break;
				}
				if (!parseHexLine(out, line))
					throw new IOException();
			}
		} catch (IOException e) {
		}
		try {
			r.close();
		} catch (IOException e1) {
		}
		return returnValue;
	}

	private boolean parseHexLine(byte[] out, String line) {
		if (line.length() < 11)
			return false;
		try {
			int dataBytes = Integer.parseInt(line.substring(1,1+2), 16);
			if (line.length() < 11 + 2 * dataBytes) 
				return false;
			int recordType = Integer.parseInt(line.substring(1+2+4,1+2+4+2), 16);
			if (recordType != 0)
				return false; // don't understand!
			
			byte sum = 0;
			for (int i = 0 ; i < 4 + dataBytes ; i++) 
				sum -= (byte)Integer.parseInt(line.substring(1+2*i,1+2*i+2), 16);

			if (sum != (byte)(Integer.parseInt(line.substring(1+2+4+2+2*dataBytes, 1+2+4+2+2*dataBytes+2),16)))
				return false;
			if (sum != (byte)Integer.parseInt(line.substring(9+2*dataBytes, 9+2*dataBytes+2),16))
				return false;
			
			int address = ( Integer.parseInt(line.substring(1+2,1+2+2), 16) << 8 ) |
					(Integer.parseInt(line.substring(1+2+2,1+2+2+2), 16));
			if (address + dataBytes > out.length)
				return false;
			byte[] data = new byte[dataBytes];
			for (int i = 0 ; i < dataBytes; i++) {
				data[i] = (byte) Integer.parseInt(line.substring(1+2+4+2+2*i,1+2+4+2+2*i+2), 16);
			}
			System.arraycopy(data, 0, out, address, dataBytes);
			return true;
		}
		catch(NumberFormatException e) {
			return false;
		}
	}
}
