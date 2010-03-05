cexsize <- 1.5 #Handles text scaling

tables.to.bytes <- function(bits, rules, numtables) {
  N <- 8 * ceiling(rules / 8)
  nt <- numtables
  even <- (nt - (bits %% nt))*(2^(bits %/% nt))*N
  odd <- (bits %% nt)*(2^((bits %/% nt) + 1))*N
  return((even + odd) / 8)
}

#returns the mean a list of data frames passed in
mean.data <- function(xs){
  ms <- merge.all(xs)
  return(aggregate.data.frame(ms
                              ,by = list(
                                 ms$number.of.rules
                                 ,ms$packet.size.in.bits
                                 ,ms$memory.used)
                              ,mean
                              )
         )
}
#pares non-numeric columns from a data.frame
rem.nonnumeric <- function(x){
  data.frame(memory.allowed = x$memory.allowed
             ,memory.used = x$memory.used
             ,number.of.rules = x$number.of.rules
             ,number.of.tables = x$number.of.tables
             ,cpu.process.time = x$cpu.process.time
             ,kbps = x$kbps
             ,packet.size.in.bits = x$packet.size.in.bits
             ,table.build.time = x$table.build.time
             ,total.run.time = x$total.run.time
             ,real.process.time = x$real.process.time
             ,policy.read.time = x$policy.read.time
             ,pps = x$pps
             )
}
#returns a data frame consisting of the merged data from all files patching the
#pattern
loadin <- function(pattern){
  f <- lapply(c(list.files(pattern = pattern)), read.csv, head=T)
  f.n <- lapply(f, rem.nonnumeric)
  return(mean.data(f.n))
}
# special version of loadin that thins out lower end of 12K graphs since they
# were tested unevenly. This should only be used for plotting, the data points
# are valid and should be included in aggregate statistics
loadin.12K <- function(pattern){
  f <- lapply(c(list.files(pattern=pattern)), read.csv, head=T)
  f.n <- lapply(f, rem.nonnumeric)
  swissed <- lapply(f.n,
                    function(x){ #trim some fat
                      x[c(  1: 50,seq( 51,101,15),
                          102:151,seq(152,202,15),
                          203:252,seq(253,303,15),
                          304:353,seq(354,404,15)
                          ),]
                    }
                    )
  return(mean.data(swissed))
}

#takes a data frame and splits it along the number.of.rules factor and returns a
#list of data frames
splitup.by <- function(x, field) {
  xlevels <- unique(x[field])
  y <- list()
  for( i in 1:length(xlevels[[1]])){
    y[[i]] <- subset(x, x[field] == xlevels[[1]][[i]], all=T)
  }
  return(y)
}
# merges all data frames in the passed in list
merge.all <- function(x) {
  merged <- x[[1]]
  if(length(x) > 1){
    for(i in 2:length(x)){
      merged <- merge(merged, x[[i]], all=T)
    }
  }
  return(merged)
}

# adds thousands separators to large numbers, suitable for graphs
comma <- function(x){
  return (formatC(x,format="d", big.mark=","))
}
# adds dots for thousands separators, suitable for filenames
dot <- function(x){
  return (formatC(x,format="d", big.mark="."))
}
# simple mnemonic functions to cut down on the number of zeroes needed to be
# read
thou <- function(x){
  return(x*1000)
}
mil <- function(x){
  return (x*1000000)
}
bil <- function(x){
  return (x*1000000000)
}
# formats with 1 significant digit
digit.1 <- function(x){
  return(formatC(x, format ="f", digit=1, drop0trailing=TRUE))
}
# adds K,M, or B suffix as needed (useful in axis labels)
sci.suffix <- function(xs){
  suffize <- function(x){
    if(x %/% bil(1) != 0){
      return(paste(digit.1(x / bil(1)), "G", sep=""))
    }
    if(x %/% mil(1) != 0){
      return(paste(digit.1(x / mil(1)), "M", sep=""))
    }
    if(x %/% thou(1) != 0){
      return(paste(digit.1(x / thou(1)), "K", sep=""))
    }#if all else fails
    return(digit.1(x))
  }
  return(lapply(xs, suffize))
}
byte.suffix <- function(xs){
  return(lapply(xs, function(x) paste(sci.suffix(x), "B",sep="")))
}
bps.suffix <- function(xs){
  return(lapply(xs, function(x) paste(sci.suffix(x), "bps", sep="")))
}

# adds a field to the data set x that contains real pps
add.real.pps <- function (x) {
  if(x$packet.size.in.bits[1] == 12000){
    x$real.pps <- thou(10) / x$real.process.time
  }else{
    x$real.pps <- thou(500) / x$real.process.time
  }
  return(x)
}

# Really should pull the "list" processing aspect out of this function, and just
# have it do one graph at a time, but currently pressed for time
real.pps.vs.tables <- function(pattern
                               ,bits
                               ,bps.line = 3
                               ,pps.line = 3
                               ,mar = c(3,4,5,4)
                               ,pch = 1
                               ,fudge = 0
                               ,tab.ticks = NULL
                               ,tab.labels = NULL
                               ,mem.ticks = NULL
                               ,mem.labels = NULL
                               ,dir="./pdfs"){
  if(bits == 12000){
    total <- loadin.12K(pattern)
  }else{
    total <- loadin(pattern)
  }
  real.pps.ed <- add.real.pps(total)
  rsp <- splitup.by(real.pps.ed, "number.of.rules")
  pps.label <- "Throughput (PPS)"
  mbps.label <- "Throughput (Mbps)"
  tab.label <- "Number of Tables"
  mem.label <- "Memory Used"
  for(i in 1:length(rsp)){
    numrules <- rsp[[i]]$number.of.rules[1]
    filename <- paste("Tables_vs_PPS_",dot(bits),"bits_",
                      dot(numrules),"rules.pdf",sep="")
    titletext <- paste("Throughputs for", comma(bits),
                       "Bits Classified, with", comma(numrules),"Rules")
    pdf(file=paste(dir,filename,sep="/"), title = titletext,
        width=11, height=8.5, paper="USr")
    par(mar=mar, cex=cexsize, bty="o")
    options(scipen=10)
    plot(x = rsp[[i]]$number.of.tables, y = rsp[[i]]$real.pps,
         xlab = "", ylab = "", xaxt="n",
         yaxt="n", tck=0, log="", pch = pch)
    if(is.null(tab.ticks)){
      tab.ticks2 <- pretty(rsp[[i]]$number.of.tables, 11)
    }else{
      tab.ticks2 <- tab.ticks[[i]]
    }
    if(is.null(tab.labels)){
      tab.labels2 <- sci.suffix(tab.ticks2)
    }else{
      tab.labels2 <- tab.labels[[i]]
    }
    if(is.null(mem.ticks)){
      mem.ticks2 <- tab.ticks2 + fudge
    }else{
      mem.ticks2 <- mem.ticks[[i]]
    }
    if(is.null(mem.labels)){
      mem.labels2 <- byte.suffix(tables.to.bytes(bits, numrules, mem.ticks2))
    }else{
      mem.labels2 <- mem.labels[[i]]
    }
    pps.ticks <- pretty(rsp[[i]]$real.pps, 10)
    axis(1, tab.ticks2, labels = tab.labels2)
    mtext(1, text = tab.label, line = 2, cex=cexsize)
    axis(2, pps.ticks, labels = sci.suffix(pps.ticks), las = 1)
    mtext(2, text = pps.label, line = pps.line, cex = cexsize)
    axis(4, at = pps.ticks, labels = pps.ticks * 0.012, las=1)
    mtext(4, text = mbps.label, line=bps.line, cex=cexsize)
    axis(3, at = mem.ticks2, labels = mem.labels2, cex.axis = 0.9)
    mtext(3, text = mem.label, line = 2, cex=cexsize)
    dev.off()
  }
}

#specified
real.pps.vs.tables.104 <- function(){
  real.pps.vs.tables(pattern = "alltest104bits_[1-3].csv"
                     ,bits = 104
                     ,pps.line = 3.2
                     ,pch = 3
                     )
}
real.pps.vs.tables.320 <- function(){
  tab.ticks <- c(26,40,55,70,85,100,115,130,145,160)
  tab.labels <- sci.suffix(tab.ticks)
  mem.ticks <- c(26,45,64,83,102,121,140,160)
  mem.labels <- byte.suffix(tables.to.bytes(320,thou(100),mem.ticks))
  real.pps.vs.tables(pattern = "alltest320bits_[1-3].csv"
                     ,bits = 320
                     ,pch = 4
                     ,fudge = 5
                     ,tab.ticks = list(1,1,1
                        ,tab.ticks
                        ,1)
                     ,tab.labels = list(1,1,1
                        ,tab.labels
                        ,1)
                     ,mem.ticks = list(1,1,1
                        ,mem.ticks
                        ,1)
                     ,mem.labels = list(1,1,1
                        ,mem.labels
                        ,1)
                     )
}
real.pps.vs.tables.12K <- function(){
  tab.ticks <- c(1160,1644,2128,2612,3096,3580,4064,4584,5068,5552,6000)
  tab.labels <- sci.suffix(tab.ticks)
  #these have been carefully crafted...
  mem.ticks <- c(1160,1764,2400,2972,3576,4180,4784,5388,6000)
  mem.labels <- byte.suffix(tables.to.bytes(thou(12), thou(10), mem.ticks))
  real.pps.vs.tables(pattern = "maxtest_[1-3].csv"
                     ,bits = 12000
                     ,bps.line = 2.3
                     ,mar = c(3,4,5,3.3)
                     ,pch = 5
                     ,tab.ticks = list(1,1
                        ,tab.ticks
                        ,1)
                     ,tab.labels = list(1,1
                        ,tab.labels
                        ,1)
                     ,mem.ticks = list(1,1
                        ,mem.ticks
                        ,1)
                     ,mem.labels = list(1,1
                        ,mem.labels
                        ,1)
                     )
}
#really... convenient
real.pps.vs.tables.all <- function(){
  #real.pps.vs.tables.104()
  real.pps.vs.tables.320()
  real.pps.vs.tables.12K()
}

classifier.throughputs <- function(dir = "./pdfs"){
  # plots the maxes and mins of different test runs
  totals <- list(  loadin("alltest32bits_[1-3].csv")
                 , loadin("alltest104bits_[1-3].csv")
                 , loadin("alltest320bits_[1-3].csv")
                 , loadin("maxtest_[1-3].csv")
                 )
  #add real.pps column to each total
  pps.ed <- lapply(totals,add.real.pps)
  #split each by number of rules
  rulesplit <- lapply(pps.ed, splitup.by, field="number.of.rules")
  #now get min and max for each rule/bit element
  minmaxed <- list()
  for( i in 1:length(rulesplit)){
    minmaxed[[i]] <- lapply(rulesplit[[i]], minmax, field="memory.used")
  }
  #remerge the results
  rmgd <- lapply(minmaxed, merge.all)
  titletext <- "Classifier Throughputs"
  #draw the plot
  pdf(paste(dir,"classifier_throughputs.pdf",sep="/"),
      width=11, height=8.5, paper="USr",
      title = titletext)
  options(scipen = 10)
  xlimit <- c(50, mil(1.5))
  ylimit <- c(100, mil(10))
  par(cex=cexsize, bty="u", mar=c(3,3,1,4.5), tcl=-0.3, ann = F)
  plot(x = rmgd[[1]]$number.of.rules, y = rmgd[[1]]$real.pps,
       xlim = xlimit, ylim = ylimit, 
       log="xy", pch = 1, xaxt="n", yaxt="n", tck = 0
       )
  pps.ticks <- c(100, thou(1), thou(10), thou(100), mil(1), mil(10))
  bps.ticks <- pps.ticks/1.2 #ticks are in pps scale
  rule.ticks <- unique(rmgd[[1]]$number.of.rules)
  bps.labels <- bps.suffix(bps.ticks*thou(12)) # labels are back in bps scale (a
                                               # packet is 12K bits)
  pps.labels <- sci.suffix(pps.ticks)
  rule.labels <- sci.suffix(rule.ticks)
  axis(1, at = rule.ticks,labels = rule.labels)
  mtext(1, text = "Number of Rules", cex = cexsize, line = 2)
  axis(2, at = pps.ticks, labels = pps.labels)
  mtext(2, text = "Throughput (PPS)", cex = cexsize, line = 2)
  axis(4, at = bps.ticks, labels = bps.labels,las=1)
  for(i in 2:length(rmgd)){
      points(x = rmgd[[i]]$number.of.rules, y = rmgd[[i]]$real.pps, pch = i+1)
  }
  legend(x="bottomleft",legend=c(32,104,320,12000),
         pch=c(1,3,4,5), title="Bits Classified")
  dev.off()
}

#returns a data set with only the minimum and maximum of the given field
minmax <- function(x, field) {
  x.min <- subset(x, x[field] == min(x[field]))[1,]
  x.max <- subset(x, x[field] == max(x[field]))[1,]
  x.final <- merge(x.min, x.max, all=T)
}

#special graph of min/max table build times for all bit sizes vs. number of rules
build.time.vs.rules <- function (dir = "./pdfs") {
  totals <- list(  loadin("alltest32bits_[1-3].csv")
                 , loadin("alltest104bits_[1-3].csv")
                 , loadin("alltest320bits_[1-3].csv")
                 , loadin("maxtest_[1-3].csv")
                )
  #split each data set up by the number of rules
  rulesplit <- lapply(totals, splitup.by, field="number.of.rules")
  #then throw away all but the min and max
  minmaxed <- list()
  for(i in 1:length(rulesplit)){
    minmaxed[[i]] <- lapply(rulesplit[[i]], minmax, field="table.build.time")
  }
  # then re-merge the results
  rmgd <- lapply(minmaxed, merge.all)
  titletext <- "Table-build Times"
  
  pdf(paste(dir,"build_time_vs_rules.pdf",sep="/"),
      height=8.5, width=11, paper="USr",
      title = titletext)
  options(scipen = 10)
  par(cex = 1.5 , bty="l",
      mar=c(3,3,1,1), ann = F)
  plot(x = rmgd[[1]]$number.of.rules, y = rmgd[[1]]$table.build.time, log="xy",
       xlim = c(100,1000000), ylim = c(0.001,2000),
       pch = 1, axes=T, xaxt="n", yaxt="n", tck=0.0)
  yticks <- c(0.001, 0.01, 0.1, 1, 10, 100, 1000)
  xticks <- c(100, thou(1), thou(10), thou(100), mil(1))
  xlabels <- sci.suffix(xticks)
  axis(1, at=xticks, labels = xlabels)
  mtext(1, text = "Number of Rules", cex = cexsize, line = 2)
  axis(2, at=yticks, labels = prettyNum(yticks, drop0trailing = T))
  mtext(2, text ="Table-build Time (s)", cex = cexsize, line = 2 )
  for( i in 2:length(rmgd)){
    points(x = rmgd[[i]]$number.of.rules, y = rmgd[[i]]$table.build.time, pch = i+1)
  }
  legend(x="bottomright",legend=c(32,104,320,12000),
         pch=c(1,3,4,5), title="Bits Classified")
  dev.off()
}

series.of.4.graph <- function(numrules
                              ,xlimits
                              ,ylimits
                              ,xlabels
                              ,bps.label = "Throughput (Gbps)"
                              ,bps.adj = 0.5
                              ,bpsmul = 0.000012
                              ,bps.line = 2
                              ,xlas = 1
                              ,mar = c(3,4,4,4)
                              ,xaxticks = F
                              ,wherelegend = "topright"
                              ,xaxis.line = 2
                              ,pps.line = 2
                              ,pps.adj = 0.5
                              ,dir="./pdfs"){
  totals <- list(  loadin("alltest32bits_[1-3].csv")
                 , loadin("alltest104bits_[1-3].csv")
                 , loadin("alltest320bits_[1-3].csv")
                 , loadin.12K("maxtest_[1-3].csv")
                 )
  titletext <- paste("Throughputs for", comma(numrules),"Rules")
  pps.ed <- lapply(totals, add.real.pps)
  only <- lapply(pps.ed, function(x) subset(x, x$number.of.rules == numrules))
  pdf(paste(dir,paste("4series_",dot(numrules),"_rules.pdf",sep=""),sep="/"),
      height=8.5, width=11, paper="USr", title = titletext)
  options(scipen = 10)# only shorten to scientific notation if difference is >
                      # 10 characters
  par(cex = cexsize , bty="u", mar = mar, ann = F)
  plot(x = only[[1]]$memory.used, y = only[[1]]$real.pps, log="x",
       xlim = xlimits, ylim = ylimits,
       pch = 1,
       xaxt="n",yaxt="n", tck=0.0
       )
  xlab <- "Memory Used"
  ppslabel <- "Throughput (PPS)"
  pps.ticks <- pretty(ylimits, 10)
  if(xaxticks[1] == F){
    axis(1, at = axTicks(1,log=T), labels = byte.suffix(axTicks(1,log=T)), las = xlas)
  }else{
    axis(1, at = xaxticks, labels = byte.suffix(xaxticks), las = xlas)
  }
  mtext(1, text = xlab, line = xaxis.line , cex = cexsize)
  axis(2, at = pps.ticks, labels = sci.suffix(pps.ticks), las=1)
  mtext(2, text = ppslabel, line = pps.line, cex = cexsize, adj = pps.adj)
  axis(4, at = pps.ticks, labels = pps.ticks * bpsmul, las = 1)
  mtext(4, text = bps.label, line = bps.line, cex = cexsize, adj = bps.adj)
  for( i in 2:length(only)){
    points(x = only[[i]]$memory.used, y = only[[i]]$real.pps, pch = i+1)
  }
  legend(x=wherelegend, legend=c(32,104,320,12000),
         pch=c(1,3,4,5), title="Bits Classified")
  dev.off()
}

#specialized for 1K
series.of.4.graph.1K <- function(){
  series.of.4.graph(numrules = 1000
                    ,xlimits = c(10000, 2000000000)
                    ,ylimits = c(1000,  1750000)
                    ,mar = c(3,4.1,3,3.8)
                    ,pps.line = 3.1
                    ,bps.line = 2.3
                    ,bps.adj = 0.45
                    )
}

series.of.4.graph.10K <- function(){
  series.of.4.graph(numrules = 10000
                    ,xlimits = c(50000, 2000000000)
                    ,ylimits = c(1, 230000)
                    ,wherelegend = "bottomleft"
                    ,mar = c(3,4,3,3.8)
                    ,pps.line = 3
                    ,bps.line = 2.8
                    )
}

series.of.4.graph.100K <- function(){
  series.of.4.graph(numrules = thou(100)
                    ,xlimits = c(mil(1), bil(2))
                    ,ylimits = c(1, thou(30))
                    ,bpsmul = 0.012
                    ,bps.label = "Throughput (Mbps)"
                    ,xaxticks = c(mil(1), mil(3), mil(10), mil(30),
                       mil(100), mil(300), bil(1))
                    ,wherelegend = "bottomleft"
                    ,xlas = 1
                    ,xaxis.line = 2
                    ,pps.line = 2.8
                    ,bps.line = 2.6
                    ,mar = c(3,3.8,3,3.6)
                    )
}
# convenience function for drawing all specialized graphs
series.of.4.graph.all <- function(){
  series.of.4.graph.100K()
  series.of.4.graph.10K()
  series.of.4.graph.1K()
}

#super convenient and slow
all.graphs <- function() {
  series.of.4.graph.all()
  real.pps.vs.tables.all()
  build.time.vs.rules()
  classifier.throughputs()
}

