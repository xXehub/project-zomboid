package mod.inventory;

import net.bytebuddy.asm.Advice;

public class Literature {

	@Advice.OnMethodExit
	public static void getNumberOfPages(@Advice.Return(readOnly = false) int numberOfPagesOg) {
		if(numberOfPagesOg > 200) {
			numberOfPagesOg = Math.max(1, Math.abs(numberOfPagesOg / 5));
		}else if (numberOfPagesOg > 100){
			numberOfPagesOg = Math.max(1, Math.abs(numberOfPagesOg / 4));			
		} else {
			numberOfPagesOg = Math.max(1, Math.abs(numberOfPagesOg / 3));
		}
	}
}
